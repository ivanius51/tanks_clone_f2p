#include "tests/EditorHistoryTests.h"

#include <memory>

#include "model/EditorHistory.h"

namespace tanks
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
		History.Execute(Document, std::make_unique<model::AddTileCommand>(model::EditorCell{ 0, 1 }, '#'));
		History.Execute(Document, std::make_unique<model::AddTileCommand>(model::EditorCell{ 0, 1 }, 'B'));
		History.Execute(Document, std::make_unique<model::RemoveTileCommand>(model::EditorCell{ 0, 1 }));
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
			History.Execute(Document, std::make_unique<model::AddTileCommand>(model::EditorCell{ 1, Column }, '#'));
		}
		InContext.Expect(History.JumpTo(Document, 1), "jump back");
		InContext.ExpectEqual(Join(Document), std::string("..../#.../..../"), "state at step 1");
		InContext.Expect(History.JumpTo(Document, 3), "jump forward");
		InContext.ExpectEqual(Join(Document), std::string("..../###./..../"), "state at step 3");
		InContext.Expect(!History.JumpTo(Document, 9), "step out of range refused");
		History.JumpTo(Document, 2);
		History.Execute(Document, std::make_unique<model::AddTileCommand>(model::EditorCell{ 2, 0 }, 'W'));
		InContext.Expect(!History.CanRedo() && History.GetSize() == 3 && History.GetStep() == 3, "new command drops the redo part");
		InContext.ExpectEqual(Join(Document), std::string("..../##../W.../"), "state after the new command");
	});

	InSuite.Add("EditorHistory: move is one undo step and keeps what it covered", [](TestContext& InContext)
	{
		model::EditorDocument Document = MakeDocument();
		model::EditorHistory History;
		History.Execute(Document, std::make_unique<model::AddTileCommand>(model::EditorCell{ 0, 0 }, 'E'));
		History.Execute(Document, std::make_unique<model::AddTileCommand>(model::EditorCell{ 2, 3 }, '#'));
		History.Execute(Document, std::make_unique<model::MoveTileCommand>(model::EditorCell{ 0, 0 }, model::EditorCell{ 2, 3 }));
		InContext.ExpectEqual(Join(Document), std::string("..../..../...E/"), "moved over the wall");
		InContext.Expect(History.GetSize() == 3, "move is a single entry");
		History.Undo(Document);
		InContext.ExpectEqual(Join(Document), std::string("E.../..../...#/"), "one undo restores both cells");
		History.Redo(Document);
		InContext.ExpectEqual(Join(Document), std::string("..../..../...E/"), "redo moves again");
		History.Execute(Document, std::make_unique<model::MoveTileCommand>(model::EditorCell{ 2, 3 }, model::EditorCell{ 2, 3 }));
		History.Undo(Document);
		InContext.ExpectEqual(Join(Document), std::string("..../..../...E/"), "moving onto itself changes nothing");
	});

	InSuite.Add("EditorHistory: a unique tile leaves its old cell and comes back on undo", [](TestContext& InContext)
	{
		model::EditorDocument Document = MakeDocument();
		model::EditorHistory History;
		History.Execute(Document, std::make_unique<model::AddTileCommand>(model::EditorCell{ 2, 0 }, 'P'));
		History.Execute(Document, std::make_unique<model::AddTileCommand>(model::EditorCell{ 0, 3 }, 'P'));
		InContext.ExpectEqual(Join(Document), std::string("...P/..../..../"), "only one start");
		History.Undo(Document);
		InContext.ExpectEqual(Join(Document), std::string("..../..../P.../"), "the start is back");
		History.Redo(Document);
		InContext.ExpectEqual(Join(Document), std::string("...P/..../..../"), "and moves again");
		History.Execute(Document, std::make_unique<model::AddTileCommand>(model::EditorCell{ 9, 9 }, '#'));
		InContext.ExpectEqual(Join(Document), std::string("...P/..../..../"), "outside the map changes nothing");
	});

	InSuite.Add("EditorHistory: properties and script snapshots undo as one entry each", [](TestContext& InContext)
	{
		model::EditorDocument Document = MakeDocument();
		model::EditorHistory History;
		History.Execute(Document, std::make_unique<model::SetPropertyCommand>("name", "Dunes"));
		History.Execute(Document, std::make_unique<model::SetPropertyCommand>("name", "Dunes 2"));
		History.Execute(Document, std::make_unique<model::SetScriptCommand>("steps: 1"));
		History.Execute(Document, std::make_unique<model::SetScriptCommand>("steps: 2"));
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
} // namespace tanks
