#include "tests/EditorLibraryTests.h"

#include "model/EditorLibrary.h"

namespace game
{
namespace tests
{

void AddEditorLibraryTests(TestSuite& InSuite)
{
	InSuite.Add("EditorLibrary: the lists hold the campaign, the custom levels and the test levels", [](TestContext& InContext)
	{
		model::CustomLibrary Custom;
		Custom.AddLevel("desert_run");
		const std::vector<model::LibraryOriginal> Campaign = { { "cfg.level.1", "configs/levels/campaign/level_1.json" } };
		const std::vector<model::LibraryOriginal> Test = { { "cfg.test_level.hits", "configs/levels/test/test_hits.json" } };

		const auto Dev = model::BuildLibraryEntries(Campaign, Test, Custom);
		InContext.Expect(Dev.size() == 3 && Dev[0].Source == model::LibrarySource::Campaign && Dev[1].Id == "desert_run" && Dev[2].Source == model::LibrarySource::Test, "dev build lists all three");
		InContext.Expect(Dev[0].Title == "level_1" && Dev[2].Title == "test_hits", "originals are named by their file");

		InContext.Expect(model::FilterLibraryEntries(Dev, model::LibrarySource::Test).size() == 1, "test levels are listed in every build");
		InContext.Expect(model::FilterLibraryEntries(Dev, model::LibrarySource::Custom).size() == 1, "filter by source");
		model::CampaignDef Own;
		Own.LevelIds = { "b", "a" };
		const auto OwnEntries = model::BuildCampaignEntries(Own);
		InContext.Expect(OwnEntries.size() == 2 && OwnEntries[0].Id == "b" && OwnEntries[1].Source == model::LibrarySource::Custom, "a campaign lists its levels in order");
	});

	InSuite.Add("EditorLibrary: an original saves into the source tree only with debug tools, a release build always saves a copy", [](TestContext& InContext)
	{
		InContext.Expect(model::ChooseSaveTarget(true, true) == model::SaveTarget::SourceFile, "dev and an original");
		InContext.Expect(model::ChooseSaveTarget(true, false) == model::SaveTarget::CustomCopy, "dev and a custom level");
		InContext.Expect(model::ChooseSaveTarget(false, true) == model::SaveTarget::CustomCopy, "release and an original");
		InContext.Expect(model::ChooseSaveTarget(false, false) == model::SaveTarget::CustomCopy, "release and a custom level");
	});

	InSuite.Add("EditorLibrary: source paths join with one slash and file titles drop folders and extension", [](TestContext& InContext)
	{
		InContext.Expect(model::JoinSourcePath("../../Resources/", "/configs/a.json") == "../../Resources/configs/a.json", "one slash");
		InContext.Expect(model::JoinSourcePath("", "configs/a.json") == "configs/a.json", "no root");
		InContext.Expect(model::GetLevelFileTitle("configs\\levels\\level_3.json") == "level_3", "backslashes");
		InContext.Expect(model::GetLevelFileTitle("plain") == "plain", "no folder, no extension");
	});
}

} // namespace tests
} // namespace game
