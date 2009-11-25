#ifndef __MODEST_MSG_EDIT_WINDOW_UI_DIMMING_PRIV_H__
#define __MODEST_MSG_EDIT_WINDOW_UI_DIMMING_PRIV_H__

#include "modest-dimming-rules-group.h"
#include "modest-ui-dimming-rules.h"

G_BEGIN_DECLS


/* Menu Dimming rules entries */
static const ModestDimmingEntry modest_msg_edit_window_menu_dimming_entries [] = {

	/* Format Menu */
	{ "/MenuBar/FormatMenu/AlignmentMenu", G_CALLBACK (modest_ui_dimming_rules_on_set_style) },
	{ "/MenuBar/FormatMenu/AlignmentLeftMenu", G_CALLBACK (modest_ui_dimming_rules_on_set_style) },
	{ "/MenuBar/FormatMenu/AlignmentRightMenu", G_CALLBACK (modest_ui_dimming_rules_on_set_style) },
	{ "/MenuBar/FormatMenu/AlignmentCenterMenu", G_CALLBACK (modest_ui_dimming_rules_on_set_style) },
	{ "/MenuBar/FormatMenu/InsertImageMenu", G_CALLBACK (modest_ui_dimming_rules_on_set_style) },
	{ "/MenuBar/EditMenu/UndoMenu", G_CALLBACK (modest_ui_dimming_rules_on_undo) },
	{ "/MenuBar/EditMenu/RedoMenu", G_CALLBACK (modest_ui_dimming_rules_on_redo) },
	{ "/MenuBar/AttachmentsMenu/RemoveAttachmentsMenu", G_CALLBACK (modest_ui_dimming_rules_on_editor_remove_attachment) },
	{ "/MenuBar/AttachmentsMenu/InsertImageMenu", G_CALLBACK (modest_ui_dimming_rules_on_set_style) },
	{ "/MenuBar/EmailMenu/SaveToDraftsMenu", G_CALLBACK (modest_ui_dimming_rules_on_save_to_drafts) },

};

/* Menu Dimming rules entries */
static const ModestDimmingEntry modest_msg_edit_window_toolbar_dimming_entries [] = {

	/* Toolbar */
	{ "/ToolBar/ToolbarSend", G_CALLBACK(modest_ui_dimming_rules_on_send) },
	{ "/ToolBar/ActionsBold", G_CALLBACK(modest_ui_dimming_rules_on_set_style) },
	{ "/ToolBar/ActionsItalics", G_CALLBACK(modest_ui_dimming_rules_on_set_style) },
};

G_END_DECLS
#endif /* __MODEST_MSG_VIEW_WINDOW_UI_PRIV_H__ */
