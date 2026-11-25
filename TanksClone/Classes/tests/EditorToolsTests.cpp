#include "tests/EditorToolsTests.h"

#include "model/EditorHistory.h"
#include "model/EditorTools.h"

namespace game
{
namespace tests
{

namespace
{
	using model::EditorCell;
	using model::EditorTool;

	void Apply(model::EditorToolController& InTools, model::EditorDocument& InDocument, model::EditorHistory& InHistory, const EditorCell& InCell, const char InTile)
	{
		if ( auto Command = InTools.Click(InDocument, InCell, InTile) )
		{
			InHistory.Execute(InDocument, std::move(Command));
		}
	}
}

void AddEditorToolsTests(TestSuite& InSuite)
{
	InSuite.Add("EditorTools: place paints, eraser and the erase item remove, nothing happens for the same tile", [](TestContext& InContext)
	{
		model::EditorDocument Document({ "...", "..." }, "P");
		model::EditorHistory History;
		model::EditorToolController Tools;
		Apply(Tools, Document, History, { 0, 0 }, '#');
		Apply(Tools, Document, History, { 0, 0 }, '#');
		InContext.Expect(History.GetSize() == 1 && Document.GetTile({ 0, 0 }) == '#', "the same tile twice is one step");
		Apply(Tools, Document, History, { 0, 0 }, '.');
		InContext.Expect(Document.GetTile({ 0, 0 }) == '.' && History.GetSize() == 2, "the erase item clears");
		Apply(Tools, Document, History, { 1, 1 }, 'S');
		Tools.SetTool(EditorTool::Eraser);
		Apply(Tools, Document, History, { 1, 1 }, 'S');
		Apply(Tools, Document, History, { 1, 2 }, 'S');
		InContext.Expect(Document.GetTile({ 1, 1 }) == '.' && Document.GetTile({ 1, 2 }) == '.' && History.GetSize() == 4, "the eraser removes objects only");
		InContext.Expect(Tools.RepeatsOnDrag(), "the eraser repeats on drag");
		Apply(Tools, Document, History, { 5, 5 }, '#');
		InContext.Expect(History.GetSize() == 4, "a click outside the map does nothing");
	});

	InSuite.Add("EditorTools: select picks an object and clears on an empty cell", [](TestContext& InContext)
	{
		model::EditorDocument Document({ "#.", ".." }, "P");
		model::EditorHistory History;
		model::EditorToolController Tools;
		Tools.SetTool(EditorTool::Select);
		Apply(Tools, Document, History, { 0, 0 }, 'x');
		InContext.Expect(Tools.HasSelection() && Tools.GetSelection() == EditorCell{ 0, 0 } && History.GetSize() == 0, "selected, no command");
		Apply(Tools, Document, History, { 1, 1 }, 'x');
		InContext.Expect(!Tools.HasSelection(), "an empty cell clears the selection");
		InContext.Expect(!Tools.RepeatsOnDrag(), "select does not repeat on drag");
	});

	InSuite.Add("EditorTools: move picks an object up, the next click puts it down as one step", [](TestContext& InContext)
	{
		model::EditorDocument Document({ "E..", "..." }, "P");
		model::EditorHistory History;
		model::EditorToolController Tools;
		InContext.Expect(!Tools.BeginMove(Document, { 1, 1 }) && Tools.GetTool() == EditorTool::Place, "an empty cell cannot be picked up");
		InContext.Expect(Tools.BeginMove(Document, { 0, 0 }) && Tools.GetTool() == EditorTool::Move && Tools.IsMovePending(), "pick up from the menu");
		Apply(Tools, Document, History, { 1, 2 }, 'x');
		InContext.Expect(Document.GetTile({ 1, 2 }) == 'E' && Document.GetTile({ 0, 0 }) == '.' && History.GetSize() == 1, "put down");
		InContext.Expect(!Tools.IsMovePending() && Tools.GetSelection() == EditorCell{ 1, 2 }, "the moved object stays selected");
		Apply(Tools, Document, History, { 1, 2 }, 'x');
		InContext.Expect(Tools.IsMovePending() && History.GetSize() == 1, "a click on an object picks it up");
		Apply(Tools, Document, History, { 1, 2 }, 'x');
		InContext.Expect(!Tools.IsMovePending() && History.GetSize() == 1, "clicking the same cell cancels");
		Tools.BeginMove(Document, { 1, 2 });
		Tools.SetTool(EditorTool::Place);
		InContext.Expect(!Tools.IsMovePending(), "changing the tool drops the pickup");
		History.Undo(Document);
		InContext.Expect(Document.GetTile({ 0, 0 }) == 'E', "undo puts it back");
	});

	InSuite.Add("EditorMenu: object lines only on an object, tools show the current one, undo and redo follow the history", [](TestContext& InContext)
	{
		model::EditorMenuContext Context;
		Context.Tool = EditorTool::Eraser;
		std::vector<model::EditorMenuItem> Items = model::BuildEditorMenu(Context);
		InContext.Expect(Items.size() == 6 && Items.front().Action == model::EditorMenuAction::ToolSelect, "an empty cell: tools, undo, redo");
		Context.bOnObject = true;
		Context.bCanUndo = true;
		Items = model::BuildEditorMenu(Context);
		InContext.Expect(Items.size() == 9 && Items[0].Action == model::EditorMenuAction::GoToCategory && Items[1].Action == model::EditorMenuAction::Remove
			&& Items[2].Action == model::EditorMenuAction::MoveObject, "on an object: go to category, remove, move first");
		InContext.Expect(Items[3].bSeparatorBefore && Items[3].Action == model::EditorMenuAction::ToolSelect, "a separator before the tools");
		int Checked = 0;
		for ( const model::EditorMenuItem& Item : Items )
		{
			Checked += Item.bChecked ? 1 : 0;
			if ( Item.bChecked )
			{
				InContext.Expect(Item.Action == model::EditorMenuAction::ToolEraser, "the current tool is checked");
			}
		}
		InContext.Expect(Checked == 1, "one tool is checked");
		InContext.Expect(Items[7].Action == model::EditorMenuAction::Undo && Items[7].bEnabled && Items[8].Action == model::EditorMenuAction::Redo && !Items[8].bEnabled, "undo enabled, redo disabled");
		model::EditorTool Tool;
		InContext.Expect(model::GetToolOfAction(model::EditorMenuAction::ToolMove, Tool) && Tool == EditorTool::Move && !model::GetToolOfAction(model::EditorMenuAction::Undo, Tool), "tool of an action");
	});
}

} // namespace tests
} // namespace game
