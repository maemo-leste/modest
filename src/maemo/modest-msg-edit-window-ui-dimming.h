#ifndef __MODEST_MSG_EDIT_WINDOW_UI_DIMMING_PRIV_H__
#define __MODEST_MSG_EDIT_WINDOW_UI_DIMMING_PRIV_H__

#include "modest-dimming-rules-group.h"
#include "modest-ui-dimming-rules.h"

G_BEGIN_DECLS


/* Menu Dimming rules entries */
static const ModestDimmingEntry modest_msg_edit_window_menu_dimming_entries [] = {

	/* Format Menu */
	{ "/MenuBar/FormatMenu/SelectFontMenu", G_CALLBACK (modest_ui_dimming_rules_on_set_style) },
	{ "/MenuBar/FormatMenu/AlignmentMenu", G_CALLBACK (modest_ui_dimming_rules_on_set_style) },
	{ "/MenuBar/FormatMenu/AlignmentLeftMenu", G_CALLBACK (modest_ui_dimming_rules_on_set_style) },
	{ "/MenuBar/FormatMenu/AlignmentRightMenu", G_CALLBACK (modest_ui_dimming_rules_on_set_style) },
	{ "/MenuBar/FormatMenu/AlignmentCenterMenu", G_CALLBACK (modest_ui_dimming_rules_on_set_style) },
	{ "/MenuBar/FormatMenu/InsertImageMenu", G_CALLBACK (modest_ui_dimming_rules_on_set_style) },	
	{ "/MenuBar/ViewMenu/ZoomMenu", G_CALLBACK (modest_ui_dimming_rules_on_zoom) },	
	{ "/MenuBar/EditMenu/SelectAllMenu", G_CALLBACK (modest_ui_dimming_rules_on_select_all) },	
	{ "/MenuBar/EditMenu/UndoMenu", G_CALLBACK (modest_ui_dimming_rules_on_undo) },	
	{ "/MenuBar/EditMenu/RedoMenu", G_CALLBACK (modest_ui_dimming_rules_on_redo) },	
	{ "/MenuBar/EditMenu/PasteMenu", G_CALLBACK (modest_ui_dimming_rules_on_editor_paste) },	
	{ "/MenuBar/AttachmentsMenu/RemoveAttachmentsMenu", G_CALLBACK (modest_ui_dimming_rules_on_editor_remove_attachment) },	
	{ "/MenuBar/EmailMenu/SaveToDraftsMenu", G_CALLBACK (modest_ui_dimming_rules_on_save_to_drafts) },

};

/* Menu Dimming rules entries */
static const ModestDimmingEntry modest_msg_edit_window_toolbar_dimming_entries [] = {

	/* Toolbar */
	{ "/ToolBar/ToolbarSend", G_CALLBACK(modest_ui_dimming_rules_on_send) },
	{ "/ToolBar/ActionsBold", G_CALLBACK(modest_ui_dimming_rules_on_set_style) },
	{ "/ToolBar/ActionsItalics", G_CALLBACK(modest_ui_dimming_rules_on_set_style) },
};

/* Clipboard Dimming rules entries */
static const ModestDimmingEntry modest_msg_edit_window_clipboard_dimming_entries [] = {

	/* Toolbar */
	{ "/MenuBar/EditMenu/CutMenu", G_CALLBACK(modest_ui_dimming_rules_on_cut) },
	{ "/MenuBar/EditMenu/CopyMenu", G_CALLBACK(modest_ui_dimming_rules_on_copy) },
	{ "/MenuBar/EditMenu/PasteMenu", G_CALLBACK(modest_ui_dimming_rules_on_editor_paste) },
};

G_END_DECLS
#endif /* __MODEST_MSG_VIEW_WINDOW_UI_PRIV_H__ */
