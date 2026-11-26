#include "tests/EditorHistoryTests.h"



#include "model/EditorHistory.h"
#include "model/EditorTools.h"
#include "templates/TUniquePtr.h"

namespace game
{
namespace tests
{

namespace
{
	model::EditorDocument MakeDocument()
	{
		return model::EditorDocument({ "....", "....", "...." }, "P");
	}

	std::string Join(const model::EditorDocument& InDocument)
	{
		std::string Result;
		for ( const std::string& Row : InDocument.GetRows() )
		{
			Result += Row + "/";
		}
		return Result;
	}
}

void AddEditorHistoryTests(TestSuite& InSuite)
{
	InSuite.Add("EditorHistory: add, remove, undo and redo restore the map", [](TestContext& InContext)
	{
		model::EditorDocument Document = MakeDocument();
		model::EditorHistory History;
		History.Execute(Document, MakeUnique<model::AddTileCommand>(model::EditorCell{ 0, 1 }, '#'));
		History.Execute(Document, MakeUnique<model::AddTileCommand>(model::EditorCell{ 0, 1 }, 'B'));
		History.Execute(Document, MakeUnique<model::RemoveTileCommand>(model::EditorCell{ 0, 1 }));
		InContext.ExpectEqual(Join(Document), std::string("..../..../..../"), "removed");
		InContext.Expect(History.Undo(Document), "undo remove");
		InContext.ExpectEqual(Join(Document), std::string(".B../..../..../"), "remove reverted");
		InContext.Expect(History.Undo(Document) && History.Undo(Document), "undo twice");
		InContext.ExpectEqual(Join(Document), std::string("..../..../..../"), "all reverted");
		InContext.Expect(!History.Undo(Document) && !History.CanUndo(), "nothing left to undo");
		InContext.Expect(History.Redo(Document) && History.Redo(Document) && History.Redo(Document), "redo three");
		InContext.ExpectEqual(Join(Document), std::string("..../..../..../"), "redo gives the same result");
		InContext.Expect(!History.Redo(Document), "nothing left to redo");
	});

	InSuite.Add("EditorHistory: a new command clears redo, jump goes to any step", [](TestContext& InContext)
	{
		model::EditorDocument Document = MakeDocument();
		model::EditorHistory History;
		for ( int Column = 0; Column < 4; ++Column )
		{
			History.Execute(Document, MakeUnique<model::AddTileCommand>(model::EditorCell{ 1, Column }, '#'));
		}
		InContext.Expect(History.JumpTo(Document, 1), "jump back");
		InContext.ExpectEqual(Join(Document), std::string("..../#.../..../"), "state at step 1");
		InContext.Expect(History.JumpTo(Document, 3), "jump forward");
		InContext.ExpectEqual(Join(Document), std::string("..../###./..../"), "state at step 3");
		InContext.Expect(!History.JumpTo(Document, 9), "step out of range refused");
		History.JumpTo(Document, 2);
		History.Execute(Document, MakeUnique<model::AddTileCommand>(model::EditorCell{ 2, 0 }, 'W'));
		InContext.Expect(!History.CanRedo() && History.GetSize() == 3 && History.GetStep() == 3, "new command drops the redo part");
		InContext.ExpectEqual(Join(Document), std::string("..../##../W.../"), "state after the new command");
	});

	InSuite.Add("EditorHistory: move is one undo step and keeps what it covered", [](TestContext& InContext)
	{
		model::EditorDocument Document = MakeDocument();
		model::EditorHistory History;
		History.Execute(Document, MakeUnique<model::AddTileCommand>(model::EditorCell{ 0, 0 }, 'E'));
		History.Execute(Document, MakeUnique<model::AddTileCommand>(model::EditorCell{ 2, 3 }, '#'));
		History.Execute(Document, MakeUnique<model::MoveTileCommand>(model::EditorCell{ 0, 0 }, model::EditorCell{ 2, 3 }));
		InContext.ExpectEqual(Join(Document), std::string("..../..../...E/"), "moved over the wall");
		InContext.Expect(History.GetSize() == 3, "move is a single entry");
		History.Undo(Document);
		InContext.ExpectEqual(Join(Document), std::string("E.../..../...#/"), "one undo restores both cells");
		History.Redo(Document);
		InContext.ExpectEqual(Join(Document), std::string("..../..../...E/"), "redo moves again");
		History.Execute(Document, MakeUnique<model::MoveTileCommand>(model::EditorCell{ 2, 3 }, model::EditorCell{ 2, 3 }));
		History.Undo(Document);
		InContext.ExpectEqual(Join(Document), std::string("..../..../...E/"), "moving onto itself changes nothing");
	});

	InSuite.Add("EditorHistory: a unique tile leaves its old cell and comes back on undo", [](TestContext& InContext)
	{
		model::EditorDocument Document = MakeDocument();
		model::EditorHistory History;
		History.Execute(Document, MakeUnique<model::AddTileCommand>(model::EditorCell{ 2, 0 }, 'P'));
		History.Execute(Document, MakeUnique<model::AddTileCommand>(model::EditorCell{ 0, 3 }, 'P'));
		InContext.ExpectEqual(Join(Document), std::string("...P/..../..../"), "only one start");
		History.Undo(Document);
		InContext.ExpectEqual(Join(Document), std::string("..../..../P.../"), "the start is back");
		History.Redo(Document);
		InContext.ExpectEqual(Join(Document), std::string("...P/..../..../"), "and moves again");
		History.Execute(Document, MakeUnique<model::AddTileCommand>(model::EditorCell{ 9, 9 }, '#'));
		InContext.ExpectEqual(Join(Document), std::string("...P/..../..../"), "outside the map changes nothing");
	});

	InSuite.Add("EditorHistory: resizing grows or cuts from the anchor, and undo brings the cut tiles back", [](TestContext& InContext)
	{
		model::EditorDocument Document({ "P.#", "...", "..#" }, "P");
		model::EditorHistory History;
		History.Execute(Document, MakeUnique<model::ResizeMapCommand>(4, 5, model::ResizeAnchor::TopLeft));
		InContext.Expect(Document.GetRowCount() == 4 && Document.GetColumnCount() == 5, "grown to 4 rows and 5 columns");
		InContext.Expect(Document.GetTile({ 0, 0 }) == 'P' && Document.GetTile({ 0, 2 }) == '#' && Document.GetTile({ 3, 4 }) == '.', "old tiles stay at the top-left, new cells are empty");
		History.Execute(Document, MakeUnique<model::ResizeMapCommand>(3, 3, model::ResizeAnchor::Center));
		InContext.Expect(Document.GetRowCount() == 3 && Document.GetColumnCount() == 3, "cut to 3x3");
		History.Execute(Document, MakeUnique<model::ResizeMapCommand>(1, 1, model::ResizeAnchor::TopLeft));
		InContext.Expect(Document.GetRowCount() == model::EditorDocument::MinSide && Document.GetColumnCount() == model::EditorDocument::MinSide, "never smaller than the format's minimum");
		History.Undo(Document);
		History.Undo(Document);
		InContext.Expect(Document.GetRowCount() == 4 && Document.GetTile({ 0, 2 }) == '#', "undo restores the first grown map");
		History.Undo(Document);
		InContext.Expect(Document.GetRowCount() == 3 && Document.GetColumnCount() == 3 && Document.GetTile({ 2, 2 }) == '#', "and the original");
		InContext.ExpectEqual(History.GetCommand(0).GetDetail(), std::string("5x4"), "detail shows columns x rows");
	});

	InSuite.Add("EditorHistory: floor paint undoes and redoes, and an empty floor is dropped", [](TestContext& InContext)
	{
		model::EditorDocument Document = MakeDocument();
		model::EditorHistory History;
		History.Execute(Document, MakeUnique<model::SetFloorCommand>(model::EditorCell{ 1, 2 }, 'w'));
		InContext.Expect(Document.GetFloorTile({ 1, 2 }) == 'w' && Document.GetFloorTile({ 0, 0 }) == '.', "the cell is painted");
		History.Execute(Document, MakeUnique<model::SetFloorCommand>(model::EditorCell{ 1, 2 }, '.'));
		InContext.Expect(Document.GetFloor().IsEmpty(), "clearing the last cell leaves no floor");
		History.Undo(Document);
		InContext.Expect(Document.GetFloorTile({ 1, 2 }) == 'w', "undo brings the paint back");
		History.Undo(Document);
		InContext.Expect(Document.GetFloor().IsEmpty(), "undo of the first paint leaves no floor");
		History.Redo(Document);
		InContext.Expect(Document.GetFloorTile({ 1, 2 }) == 'w', "redo paints again");
		InContext.ExpectEqual(std::string(model::GetHistoryTextKey(History.GetCommand(0).GetKind())), std::string("EDITOR_HISTORY_FLOOR"), "has its own history text");
	});

	InSuite.Add("EditorHistory: points place, move, remove and undo; a resize keeps floor and points", [](TestContext& InContext)
	{
		model::EditorDocument Document = MakeDocument();
		model::EditorHistory History;
		History.Execute(Document, MakeUnique<model::SetPointCommand>(model::EditorCell{ 0, 1 }, "spawn_a"));
		History.Execute(Document, MakeUnique<model::SetPointCommand>(model::EditorCell{ 2, 3 }, "spawn_a"));
		const model::LevelPoint* const Moved = Document.GetLayers().FindPoint("spawn_a");
		InContext.Expect(Moved && Moved->Column == 3 && Moved->Row == 2 && Document.GetLayers().GetPoints().Size() == 1, "placing the same name moves it");
		History.Execute(Document, MakeUnique<model::SetPointCommand>(model::EditorCell{ 2, 3 }, std::string()));
		InContext.Expect(Document.GetLayers().GetPoints().IsEmpty(), "an empty name removes the point under the cell");
		History.Undo(Document);
		InContext.Expect(Document.GetLayers().FindPointAt(3, 2) != nullptr, "undo of the removal brings it back");
		History.Execute(Document, MakeUnique<model::SetFloorCommand>(model::EditorCell{ 0, 0 }, 'f'));
		History.Execute(Document, MakeUnique<model::ResizeMapCommand>(3, 3, model::ResizeAnchor::TopLeft));
		InContext.Expect(Document.GetLayers().FindPoint("spawn_a") == nullptr && Document.GetFloorTile({ 0, 0 }) == 'f' && Document.GetFloor()[0].size() == 3, "the cut drops the point outside and the floor follows the size");
		History.Undo(Document);
		InContext.Expect(Document.GetLayers().FindPoint("spawn_a") != nullptr && Document.GetFloor()[0].size() == 4, "undo of the resize restores both");
		InContext.ExpectEqual(std::string(model::GetHistoryTextKey(History.GetCommand(0).GetKind())), std::string("EDITOR_HISTORY_POINT"), "has its own history text");
	});

	InSuite.Add("EditorHistory: properties and script snapshots undo as one entry each", [](TestContext& InContext)
	{
		model::EditorDocument Document = MakeDocument();
		model::EditorHistory History;
		History.Execute(Document, MakeUnique<model::SetPropertyCommand>("name", "Dunes"));
		History.Execute(Document, MakeUnique<model::SetPropertyCommand>("name", "Dunes 2"));
		History.Execute(Document, MakeUnique<model::SetScriptCommand>("steps: 1"));
		History.Execute(Document, MakeUnique<model::SetScriptCommand>("steps: 2"));
		InContext.ExpectEqual(Document.GetScriptText(), std::string("steps: 2"), "script set");
		History.Undo(Document);
		InContext.ExpectEqual(Document.GetScriptText(), std::string("steps: 1"), "whole script restored");
		History.Undo(Document);
		InContext.ExpectEqual(Document.GetScriptText(), std::string(""), "empty script restored");
		History.Undo(Document);
		InContext.ExpectEqual(Document.GetProperty("name"), std::string("Dunes"), "property restored");
		InContext.Expect(History.GetCommand(0).GetKind() == model::EditorCommandKind::Property && History.GetCommand(2).GetKind() == model::EditorCommandKind::Script, "kinds for the history list");
		InContext.ExpectEqual(History.GetCommand(0).GetDetail(), std::string("name"), "detail names the property");
	});
}

} // namespace tests
} // namespace game
