<#
Finds and removes binaries (build output, libraries, big files) from the git repository.

Actions:
  Report    the largest files in the whole history, grouped by path (-SizeMB sets the minimum, default 1)
  Tracked   build output that is tracked right now (exe, dll, pdb, obj, lib, ...) and the big tracked files
  Untrack   stops tracking the build output listed by Tracked (git rm --cached; files stay on disk, nothing is committed)
  Rewrite   removes build output from the WHOLE history with git filter-repo to shrink the repository.
            Dry run by default: it only lists what would go. Add -Execute to do it (needs git-filter-repo:
            pip install git-filter-repo). It first writes a backup bundle next to the repository.
            Rewriting changes every commit id: afterwards re-add the remote and force-push yourself, and every
            clone must be re-cloned. This script never pushes.

Usage:
  tools\repo_clean.ps1 -Action Report -SizeMB 5
  tools\repo_clean.ps1 -Action Tracked
  tools\repo_clean.ps1 -Action Untrack
  tools\repo_clean.ps1 -Action Rewrite              # dry run
  tools\repo_clean.ps1 -Action Rewrite -Execute
  tools\repo_clean.ps1 -Action Rewrite -Glob 'TanksClone/cocos2d/external/*/prebuilt/*' -Execute   # extra path patterns
#>
param(
	[Parameter(Mandatory = $true)][ValidateSet('Report', 'Tracked', 'Untrack', 'Rewrite')][string]$Action,
	[double]$SizeMB = 1,
	[string[]]$Glob = @(),
	[switch]$Execute
)

. "$PSScriptRoot\common.ps1"

$gitRoot = Get-GitRoot
# Build output and intermediate files that never belong in the repository.
$buildGlobs = @(
	'*/Debug.win32/*', '*/Release.win32/*', '*/Test.win32/*', '*/dist/*',
	'*.exe', '*.dll', '*.pdb', '*.obj', '*.ilk', '*.idb', '*.exp', '*.pch', '*.ipch', '*.lib', '*.tlog', '*.log', '*.zip'
)
# Third-party binaries that are meant to be kept (prebuilt engine libraries).
$keepPrefixes = @('TanksClone/cocos2d/external/', 'TanksClone/cocos2d/tools/')

function Test-BuildPath([string]$Path, [string[]]$Patterns) {
	foreach ($pattern in $Patterns) { if ($Path -like $pattern) { return $true } }
	return $false
}

function Get-HistoryBlobs {
	git -C $gitRoot rev-list --objects --all |
		git -C $gitRoot cat-file "--batch-check=%(objecttype) %(objectname) %(objectsize) %(rest)" |
		ForEach-Object {
			$parts = $_ -split ' ', 4
			if ($parts[0] -eq 'blob' -and $parts.Count -ge 4) { [pscustomobject]@{ Size = [int64]$parts[2]; Path = $parts[3] } }
		}
}

function Format-Size([int64]$Bytes) {
	if ($Bytes -ge 1MB) { return ('{0,8:N1} MB' -f ($Bytes / 1MB)) }
	return ('{0,8:N0} KB' -f ($Bytes / 1KB))
}

function Get-TrackedBuildFiles {
	git -C $gitRoot ls-files | Where-Object { Test-BuildPath $_ $buildGlobs } | Where-Object { -not (Test-BuildPath $_ $keepPrefixes.ForEach({ "$_*" })) }
}

switch ($Action) {
	'Report' {
		Write-Step "Files over $SizeMB MB in the history (all versions of a path added together)"
		$limit = [int64]($SizeMB * 1MB)
		Get-HistoryBlobs | Where-Object { $_.Size -ge $limit } | Group-Object Path |
			ForEach-Object { [pscustomobject]@{ Total = ($_.Group | Measure-Object Size -Sum).Sum; Versions = $_.Count; Path = $_.Name } } |
			Sort-Object Total -Descending | Select-Object -First 40 |
			ForEach-Object { '{0} x{1,-3} {2}' -f (Format-Size $_.Total), $_.Versions, $_.Path }
		git -C $gitRoot count-objects -vH | Select-String 'size-pack|in-pack' | ForEach-Object { Write-Host $_.Line }
	}

	'Tracked' {
		Write-Step 'Build output tracked right now'
		$files = @(Get-TrackedBuildFiles)
		if ($files.Count -eq 0) { Write-Ok 'none' } else { $files | ForEach-Object { Write-Host $_ }; Write-Bad "$($files.Count) file(s)" }
		Write-Step "Tracked files over $SizeMB MB"
		$limit = [int64]($SizeMB * 1MB)
		git -C $gitRoot ls-files | ForEach-Object { Get-Item (Join-Path $gitRoot $_) -ErrorAction SilentlyContinue } |
			Where-Object { $_ -and $_.Length -ge $limit } | Sort-Object Length -Descending |
			ForEach-Object { '{0} {1}' -f (Format-Size $_.Length), $_.FullName.Substring($gitRoot.Length + 1) }
	}

	'Untrack' {
		$files = @(Get-TrackedBuildFiles)
		if ($files.Count -eq 0) { Write-Ok 'nothing to untrack'; break }
		Write-Step "Untracking $($files.Count) file(s) (they stay on disk; commit the removal yourself)"
		$files | ForEach-Object { git -C $gitRoot rm --cached -q -- $_ }
	}

	'Rewrite' {
		$patterns = $buildGlobs + $Glob
		Write-Step 'Paths in the history that would be removed'
		$matches = @(Get-HistoryBlobs | Where-Object { (Test-BuildPath $_.Path $patterns) -and -not (Test-BuildPath $_.Path $keepPrefixes.ForEach({ "$_*" })) } |
			Group-Object Path | ForEach-Object { [pscustomobject]@{ Path = $_.Name; Total = ($_.Group | Measure-Object Size -Sum).Sum } })
		$total = ($matches | Measure-Object Total -Sum).Sum
		$matches | Sort-Object Total -Descending | Select-Object -First 25 | ForEach-Object { '{0} {1}' -f (Format-Size $_.Total), $_.Path }
		Write-Host ("{0} path(s), about {1} before compression" -f $matches.Count, (Format-Size ([int64]$total)).Trim())
		if (-not $Execute) { Write-Host 'Dry run. Add -Execute to rewrite the history.' -ForegroundColor Yellow; break }

		git -C $gitRoot filter-repo --version *> $null
		if ($LASTEXITCODE -ne 0) { throw 'git-filter-repo is not installed: pip install git-filter-repo' }
		if (git -C $gitRoot status --porcelain) { throw 'the working tree has uncommitted changes: commit or stash them first' }

		$bundle = Join-Path (Split-Path $gitRoot -Parent) ("backup_{0}.bundle" -f (Get-Date -Format 'yyyyMMdd_HHmmss'))
		Write-Step "Backup bundle: $bundle"
		git -C $gitRoot bundle create $bundle --all
		if ($LASTEXITCODE -ne 0) { throw 'could not write the backup bundle' }

		$remote = git -C $gitRoot remote get-url origin 2>$null
		$filterArguments = @('filter-repo', '--invert-paths', '--force')
		foreach ($pattern in $patterns) { $filterArguments += @('--path-glob', $pattern) }
		Write-Step 'Rewriting the history'
		git -C $gitRoot @filterArguments
		if ($LASTEXITCODE -ne 0) { throw 'git filter-repo failed (the backup bundle is intact)' }
		git -C $gitRoot reflog expire --expire=now --all
		git -C $gitRoot gc --prune=now --aggressive
		Write-Ok 'History rewritten.'
		if ($remote) { Write-Host "filter-repo removed the remote. To publish: git remote add origin $remote ; git push --force --all ; git push --force --tags" -ForegroundColor Yellow }
	}
}
