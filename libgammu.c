#include <glib.h>
#include <gammu.h>

#include <prpl.h>
#include <account.h>
#include <blist.h>
#include <status.h>
#include <accountopt.h>

#define GAMMU_PLUGIN_VERSION "0.1"
#define GAMMU_PLUGIN_ID "prpl-bigbrownchunx-gammu"

void gam_got_sms(GSM_StateMachine *sm, GSM_SMSMessage sms, void *user_data)
{
	PurpleConnection *pc = user_data;
}

void gam_send_sms_cb(GSM_StateMachine *sm, int status,
				       int MessageReference, void *user_data)
{
	PurpleConnection *pc = user_data;
}				       

const char *gam_list_icon(PurpleAccount *account, PurpleBuddy *buddy)
{
	return "gammu";
}

GList *gam_status_types(PurpleAccount *account)
{
	GList *types = NULL;
	PurpleStatusType *status;
	
	//Only online
	status = purple_status_type_new_full(PURPLE_STATUS_AVAILABLE, NULL, "Online", FALSE, TRUE, FALSE);
	types = g_list_append(types, status);

	return types;
}

void gam_login(PurpleAccount *account)
{
	PurpleConnection *pc;
	GSM_StateMachine *sm;
	int err;
	gchar imei[30];
	
	pc = purple_account_get_connection(account);
	
	sm = GSM_AllocStateMachine();
	pc->proto_data = sm;
	
	err = GSM_InitConnection(sm, 1);
	if (err == ERR_NONE) {
		GSM_SetSendSMSStatusCallback(sm, gam_send_sms_cb, pc);
		GSM_SetIncomingSMSCallback(sm, gam_got_sms, pc);
		GSM_SetFastSMSSending(sm, TRUE);
		//GSM_GetIMEI(sm, &imei);
		purple_connection_set_state(pc, PURPLE_CONNECTED);
	}
	
}

void gam_close(PurpleConnection *pc)
{
	GSM_StateMachine *sm = pc->proto_data;
	
	if (GSM_IsConnected(sm))
		GSM_TerminateConnection(sm);
	GSM_FreeStateMachine(sm);
}

int gam_send_im(PurpleConnection *pc, const char *who, const char *message, PurpleMessageFlags flags)
{
	GSM_MultiSMSMessage *sms;
	GSM_MultiPartSMSInfo info;
	
	GSM_ClearMultiPartSMSInfo(&info);
}

static gboolean plugin_load(PurplePlugin *plugin)
{
	return TRUE;
}

static gboolean plugin_unload(PurplePlugin *plugin)
{
	return TRUE;
}

static void plugin_init(PurplePlugin *plugin)
{
	PurpleAccountOption *option;
	PurplePluginInfo *info = plugin->info;
	PurplePluginProtocolInfo *prpl_info = info->extra_info;

//	option = purple_account_option_bool_new(
//		_("Edit Facebook friends from Pidgin"),
//		"facebook_manage_friends", FALSE);
//	prpl_info->protocol_options = g_list_append(
//		prpl_info->protocol_options, option);
}

static PurplePluginProtocolInfo prpl_info = {
	/* options */
	OPT_PROTO_PASSWORD_OPTIONAL,

	NULL,                   /* user_splits */
	NULL,                   /* protocol_options */
	/* NO_BUDDY_ICONS */    /* icon_spec */
	{"jpg", 0, 0, 50, 50, -1, PURPLE_ICON_SCALE_SEND}, /* icon_spec */
	gam_list_icon,          /* list_icon */
	NULL,                   /* list_emblems */
	NULL,                   /* status_text */
	NULL,                   /* tooltip_text */
	gam_status_types,       /* status_types */
	NULL,                   /* blist_node_menu */
	NULL,                   /* chat_info */
	NULL,                   /* chat_info_defaults */
	gam_login,              /* login */
	gam_close,              /* close */
	gam_send_im,            /* send_im */
	NULL,                   /* set_info */
	NULL,                   /* send_typing */
	NULL,                   /* get_info */
	NULL,                   /* set_status */
	NULL,                   /* set_idle */
	NULL,                   /* change_passwd */
	NULL,                   /* add_buddy */
	NULL,                   /* add_buddies */
	NULL,                   /* remove_buddy */
	NULL,                   /* remove_buddies */
	NULL,                   /* add_permit */
	NULL,                   /* add_deny */
	NULL,                   /* rem_permit */
	NULL,                   /* rem_deny */
	NULL,                   /* set_permit_deny */
	NULL,                   /* join_chat */
	NULL,                   /* reject chat invite */
	NULL,                   /* get_chat_name */
	NULL,                   /* chat_invite */
	NULL,                   /* chat_leave */
	NULL,                   /* chat_whisper */
	NULL,                   /* chat_send */
	NULL,                   /* keepalive */
	NULL,                   /* register_user */
	NULL,                   /* get_cb_info */
	NULL,                   /* get_cb_away */
	NULL,                   /* alias_buddy */
	NULL,                   /* group_buddy */
	NULL,                   /* rename_group */
	NULL,                   /* buddy_free */
	NULL,                   /* convo_closed */
	purple_normalize_nocase,/* normalize */
	NULL,                   /* set_buddy_icon */
	NULL,                   /* remove_group */
	NULL,                   /* get_cb_real_name */
	NULL,                   /* set_chat_topic */
	NULL,                   /* find_blist_chat */
	NULL,                   /* roomlist_get_list */
	NULL,                   /* roomlist_cancel */
	NULL,                   /* roomlist_expand_category */
	NULL,                   /* can_receive_file */
	NULL,                   /* send_file */
	NULL,                   /* new_xfer */
	NULL,                   /* offline_message */
	NULL,                   /* whiteboard_prpl_ops */
	NULL,                   /* send_raw */
	NULL,                   /* roomlist_room_serialize */
	NULL,                   /* unregister_user */
	NULL,                   /* send_attention */
	NULL,                   /* attention_types */
#if PURPLE_MAJOR_VERSION >= 2 && PURPLE_MINOR_VERSION >= 5
	sizeof(PurplePluginProtocolInfo), /* struct_size */
	NULL                    /* get_account_text_table */
#else
	(gpointer) sizeof(PurplePluginProtocolInfo)
#endif
};

static PurplePluginInfo info = {
	PURPLE_PLUGIN_MAGIC,
	2,						/* major_version */
	3, 						/* minor version */
	PURPLE_PLUGIN_PROTOCOL, 			/* type */
	NULL, 						/* ui_requirement */
	0, 						/* flags */
	NULL, 						/* dependencies */
	PURPLE_PRIORITY_DEFAULT, 			/* priority */
	GAMMU_PLUGIN_ID,				/* id */
	"Gammu (SMS)", 					/* name */
	GAMMU_PLUGIN_VERSION,	 			/* version */
	("Gammu Protocol Plugin"),	 		/* summary */
	("Gammu Protocol Plugin"),	 		/* description */
	"Eion Robb <eionrobb@gmail.com>", 		/* author */
	"http://purple-gammu.googlecode.com/",		/* homepage */
	plugin_load, 					/* load */
	plugin_unload, 					/* unload */
	NULL, 						/* destroy */
	NULL, 						/* ui_info */
	&prpl_info, 					/* extra_info */
	NULL, 						/* prefs_info */
	NULL,	 					/* actions */

							/* padding */
	NULL,
	NULL,
	NULL,
	NULL
};

PURPLE_INIT_PLUGIN(gammu, plugin_init, info);
