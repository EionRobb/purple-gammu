/*
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
                
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
                                
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <string.h>
#include <glib.h>
#include <gammu.h>

#include <prpl.h>
#include <account.h>
#include <blist.h>
#include <status.h>
#include <accountopt.h>
#include <debug.h>

#define GAMMU_PLUGIN_VERSION "0.2"
#define GAMMU_PLUGIN_ID "prpl-bigbrownchunx-gammu"

typedef struct {
	GSM_StateMachine *sm;
	guint readDeviceTimeout;
} GammuProtoData;

gboolean
gam_read_device(gpointer userdata)
{
	GSM_StateMachine *sm = userdata;
	
	GSM_ReadDevice(sm, FALSE);

	return GSM_IsConnected(sm);
}

void
gam_debug_func(const char *text, void *data)
{
	purple_debug_info("gammu", "%s\n", text);
}

void gam_error(GSM_Error err)
{
	if (err == ERR_NONE)
		return;
	
	purple_debug_error("gammu", "Failure: %s\n", GSM_ErrorString(err));
}

void gam_keepalive(PurpleConnection *pc)
{
	GSM_StateMachine *sm = pc->proto_data;

	GSM_ReadDevice(sm, FALSE);
}

void gam_got_sms(GSM_StateMachine *sm, GSM_SMSMessage sms, void *user_data)
{
	PurpleConnection *pc = user_data;
	gchar *sender;
	gchar *message;
	
	sender = DecodeUnicodeString(sms.Number);
	if (sms.Coding == SMS_Coding_8bit)
		//message = g_strdup("8-bit message, can not display");
		message = g_strdup(sms.Text);
	else
		message = DecodeUnicodeString(sms.Text);
	
	serv_got_im(pc, sender, message, PURPLE_MESSAGE_RECV, time(NULL));
	
	g_free(sender);
	g_free(message);
}

void gam_send_sms_cb(GSM_StateMachine *sm, int status,
				       int MessageReference, void *user_data)
{
	PurpleConnection *pc = user_data;
	if (status == 0)
		purple_debug_info("gammu", "Message sent ok\n");
	else
		purple_debug_error("gammu", "Message was not sent\n");
}				       

const char *gam_list_icon(PurpleAccount *account, PurpleBuddy *buddy)
{
	return "gammu";
}

const char *gam_list_emblem(PurpleBuddy *buddy)
{
	return "mobile";
}

void gam_make_online(PurpleAccount *account, const char* name)
{
	GSList *buddies = purple_find_buddies(account, name);
	PurpleBuddy *buddy;

	for(; buddies; buddies = g_slist_delete_link(buddies, buddies))
	{
		buddy = buddies->data;
		purple_prpl_got_user_status(account, buddy->name, purple_primitive_get_id_from_type(PURPLE_STATUS_AVAILABLE), NULL);
	}	
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

void gam_add_buddy(PurpleConnection *pc, PurpleBuddy *buddy, PurpleGroup *group)
{
	//TODO add to phone contact list
	gam_make_online(pc->account, buddy->name);
}

gboolean gam_check_pin(PurpleConnection *pc)
{
	GSM_StateMachine *sm = pc->proto_data;
	GSM_SecurityCode code;
	GSM_Error err;
	const char *password = NULL;
	PurpleAccount *account;
	
	if (sm == NULL)
		return FALSE;
	
	err = GSM_GetSecurityStatus(sm, &code.Type);
	if (err == ERR_NOTSUPPORTED) {
		return TRUE;
	}
	
	if (err != ERR_NONE)
		return FALSE;
	
	switch (code.Type) {
		case SEC_None:
			return TRUE;
		case SEC_Pin:
			//use PIN code
			break;
		case SEC_Phone:
			//use phone code
			break;
		case SEC_Network:
			//use network code
			break;
		case SEC_SecurityCode:
		case SEC_Pin2:
		case SEC_Puk:
		case SEC_Puk2:
			//not sure
			return FALSE;
	}
	
	account = purple_connection_get_account(pc);
	password = purple_account_get_password(account);
	
	if (!password || password[0] == '\0')
		return FALSE;
	
	g_stpcpy(code.Code, password);
	err = GSM_EnterSecurityCode(sm, code);
	if (err == ERR_SECURITYERROR)
	{
		//wrong pin
		return FALSE;
	}
	if (err != ERR_NONE)
	{
		//some other error
		return FALSE;
	}
	
	return TRUE;
}

void gam_download_buddies(PurpleConnection *pc)
{
	GSM_StateMachine *sm = pc->proto_data;
	GSM_MemoryEntry entry;
	GSM_Error error = ERR_NONE;
	PurpleAccount *account = pc->account;
	gchar *text;
	int i;
	
	gchar *name;
	gchar *number;
	
	entry.MemoryType = MEM_ME; //Read from internal memory
	entry.Location = 0;
	
	for(error = GSM_GetNextMemory(sm, &entry, TRUE);
		error == ERR_NONE;
		error = GSM_GetNextMemory(sm, &entry, FALSE))
	{
		name = NULL;
		number = NULL;		
		purple_debug_info("gammu", "buddy %d\n", entry.Location);
		for(i = 0; i < entry.EntriesNum; i++)
		{
			purple_debug_info("gammu", "entry type %d\n", entry.Entries[i].EntryType);
			switch(entry.Entries[i].EntryType) {
				case PBK_Number_General:
				case PBK_Number_Mobile:
				case PBK_Number_Mobile_Work:
				case PBK_Number_Mobile_Home:
					if (number == NULL)
					{
						number = DecodeUnicodeString(entry.Entries[i].Text);
					}
					break;
				case PBK_Text_Name:
				case PBK_Text_FirstName:
					if (name == NULL)
					{
						name = DecodeUnicodeString(entry.Entries[i].Text);
					}
					break;
				case PBK_Photo:
					purple_debug_info("gammu", "Found a photo for location %d!\n", entry.Entries[i].Number);
					//GSM_BinaryPicture picture = entry.Entries[i].Picture;
					break;
				default:
					break;
			}
		}
		if (name && number && !purple_find_buddy(account, number))
		{
			PurpleBuddy *buddy = purple_buddy_new(account, number, name);
			purple_blist_add_buddy(buddy, NULL, NULL, NULL);
		}
		g_free(name);
		g_free(number);

		GSM_FreeMemoryEntry(&entry);
	}
	gam_error(error);
}

void gam_login(PurpleAccount *account)
{
	GSM_Config *gammucfg;
	PurpleConnection *pc;
	GSM_StateMachine *sm;
	int err;
	gchar buffer[100];
	
	pc = purple_account_get_connection(account);
	
	GSM_InitLocales(NULL);
	
	sm = GSM_AllocStateMachine();
	pc->proto_data = sm;

	gammucfg = GSM_GetConfig(sm, 0);
	
	gammucfg->Device = g_strdup(purple_account_get_string(account, "port", "com5:"));
	gammucfg->Connection = g_strdup(purple_account_get_string(account, "connection", "at"));
	g_stpcpy(gammucfg->Model, purple_account_get_string(account, "model", "auto"));
	
	GSM_SetConfigNum(sm, 1);
	gammucfg->UseGlobalDebugFile = FALSE;
	
	err = GSM_InitConnection(sm, 1);
	if (err == ERR_NONE) {
		GSM_SetSendSMSStatusCallback(sm, gam_send_sms_cb, pc);
		GSM_SetIncomingSMS(sm, TRUE);
		GSM_SetIncomingSMSCallback(sm, gam_got_sms, pc);
		GSM_SetFastSMSSending(sm, TRUE);
		if (gam_check_pin(pc))
		{
			purple_connection_update_progress(pc, "Done", 1, 1);
			purple_connection_set_state(pc, PURPLE_CONNECTED);
			
		} else {
			purple_connection_error_reason(pc,
				PURPLE_CONNECTION_ERROR_AUTHENTICATION_FAILED,
				"Incorrect PIN Number");
			return;
		}
	} else {
		purple_connection_error_reason(pc,
			PURPLE_CONNECTION_ERROR_NETWORK_ERROR,
			"Could not connect to phone");
		return;
	}

	GSM_GetManufacturer(sm, buffer);
	purple_debug_info("gammu", "Manufacturer: %s\n", buffer);
	GSM_GetModel(sm, buffer);
	purple_debug_info("gammu", "Model: %s (%s)\n", GSM_GetModelInfo(sm)->model, buffer);

	if (purple_account_get_bool(account, "download_phonebook", FALSE))
	{
		gam_download_buddies(pc);
	}
	gam_make_online(account, NULL);
			
	purple_timeout_add_seconds(1, gam_read_device, sm);
}

void gam_close(PurpleConnection *pc)
{
	GSM_StateMachine *sm = pc->proto_data;
	
	if (GSM_IsConnected(sm))
	{
		purple_debug_info("gammu", "Closing connection");
		GSM_TerminateConnection(sm);
		if (GSM_IsConnected(sm))
			GSM_TerminateConnection(sm);
	}
	GSM_FreeStateMachine(sm);
	pc->proto_data = NULL;
}

int gam_send_im(PurpleConnection *pc, const char *who, const char *message, PurpleMessageFlags flags)
{
	//GSM_MultiSMSMessage sms;
	//GSM_MultiPartSMSInfo info;
	GSM_StateMachine *sm = pc->proto_data;
	GSM_SMSC PhoneSMSC;
	//int i;
	GSM_SMSMessage *sms;
	gint encoding;

	if (!GSM_IsConnected(sm))
	{
		return -1;
	}
	
	encoding = atoi(purple_account_get_string(pc->account, "encoding", "1"));

	//for (i = 0; i < GSM_MAX_MULTI_SMS; i++) {
	//	GSM_SetDefaultSMSData(&sms.SMS[i]);
	//}
	sms = g_new0(GSM_SMSMessage, 1);
	
	EncodeUnicode(sms->Text, message, strlen(message));
	EncodeUnicode(sms->Number, who, strlen(who));
	sms->PDU = SMS_Submit;
	sms->UDH.Type = UDH_NoUDH;

	sms->Coding = encoding;
	sms->Class = 1;
	
	// Set this SMS SMSC using the phones SMSC
	PhoneSMSC.Location = 1;
	GSM_GetSMSC(sm, &PhoneSMSC);
	CopyUnicodeString(sms->SMSC.Number, PhoneSMSC.Number);

	GSM_SendSMS(sm, sms);
	
	//GSM_ClearMultiPartSMSInfo(&info);
	g_free(sms);
	
	return 1;
}

#if PURPLE_MAJOR_VERSION >= 2 && PURPLE_MINOR_VERSION >= 5
static GHashTable *gam_account_text_table(PurpleAccount *account)
{
	GHashTable *table;
	
	table = g_hash_table_new(g_str_hash, g_str_equal);
	
	g_hash_table_insert(table, "login_label", (gpointer)"Anything"));
	g_hash_table_insert(table, "password_label", (gpointer)"PIN Number"));
	
	return table;
}
#endif

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

	GList *encoding_list = NULL;
	GList *model_list = NULL;
	GList *connection_list = NULL;
	PurpleKeyValuePair *kvp;

	option = purple_account_option_string_new(
		"Port/Device", "port", "com5:");
	prpl_info->protocol_options = g_list_append(
		prpl_info->protocol_options, option);
	

	//Connection
#define add_connection(x)	kvp = g_new0(PurpleKeyValuePair, 1); \
							kvp->key = g_strdup( x ); \
							kvp->value = g_strdup( x ); \
							connection_list = g_list_append(connection_list, kvp);
	add_connection("at");
	add_connection("at19200");
	add_connection("at115200");
	add_connection("fbus");
	add_connection("fbusirda");
	add_connection("fbusdlr3");
	add_connection("fbusdku5");
	add_connection("fbususb");
	add_connection("dku2phonet");
	add_connection("dku5fbus2");
	add_connection("fbuspl2303");
	add_connection("mbus");
	add_connection("irdaphonet");
	add_connection("irdaat");
	add_connection("irdaobex");
	add_connection("irdagnapbus");
	add_connection("bluerffbus");
	add_connection("bluefbus");
	add_connection("bluerfphonet");
	add_connection("bluephonet");
	add_connection("blueat");
	add_connection("bluerfat");
	add_connection("blueobex");
	add_connection("bluerfobex");
	add_connection("phonetblue");
	add_connection("fbusblue");
	add_connection("bluefgnapbus");
	
	option = purple_account_option_list_new(
		"Connection", "connection", connection_list);
	prpl_info->protocol_options = g_list_append(
		prpl_info->protocol_options, option);

	//Model
#define add_model(x)	kvp = g_new0(PurpleKeyValuePair, 1); \
						kvp->key = g_strdup( x ); \
						kvp->value = g_strdup( x ); \
						model_list = g_list_append(model_list, kvp);
	add_model("auto");
	add_model("at");
	add_model("alcatel");
	add_model("nauto");
	add_model("obex");
	add_model("seobex");
	
	option = purple_account_option_list_new(
		"Model", "model", model_list);
	prpl_info->protocol_options = g_list_append(
		prpl_info->protocol_options, option);

	// Encoding
#define add_encoding(x, y)	kvp = g_new0(PurpleKeyValuePair, 1); \
							kvp->key = g_strdup( x ); \
							kvp->value = g_strdup_printf("%d", y); \
							encoding_list = g_list_append(encoding_list, kvp);
	add_encoding("Default", SMS_Coding_Default_No_Compression);
	add_encoding("Default, Compressed", SMS_Coding_Default_Compression);
	add_encoding("Unicode", SMS_Coding_Unicode_No_Compression);
	add_encoding("Unicode, Compressed", SMS_Coding_Unicode_Compression);
	add_encoding("8-bit", SMS_Coding_8bit);
	
	option = purple_account_option_list_new(
		"SMS Encoding", "encoding",
		encoding_list);
	prpl_info->protocol_options = g_list_append(
		prpl_info->protocol_options, option);

	option = purple_account_option_bool_new(
		"Download phonebook", "download_phonebook", FALSE);
	prpl_info->protocol_options = g_list_append(
		prpl_info->protocol_options, option);

	GSM_SetDebugFunction(gam_debug_func, NULL, GSM_GetGlobalDebug());	
}

static PurplePluginProtocolInfo prpl_info = {
	/* options */
	OPT_PROTO_PASSWORD_OPTIONAL,

	NULL,                   /* user_splits */
	NULL,                   /* protocol_options */
	/* NO_BUDDY_ICONS */    /* icon_spec */
	{"jpg", 0, 0, 50, 50, -1, PURPLE_ICON_SCALE_SEND}, /* icon_spec */
	gam_list_icon,          /* list_icon */
	gam_list_emblem,        /* list_emblems */
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
	gam_keepalive,          /* keepalive */
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
	gam_account_text_table  /* get_account_text_table */
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


#if defined(__MINGW32__) || defined(__APPLE__)
#include <ctype.h>
char *
strcasestr(char *haystack, char *needle)
{
	char *p, *startn = 0, *np = 0;
	for (p = haystack; *p; p++) {
		if (np) {
			if (toupper(*p) == toupper(*np)) {
				if (!*++np)
					return startn;
			} else
				np = 0;
		} else if (toupper(*p) == toupper(*needle)) {
			np = needle + 1;
			startn = p;
		}
	}
	return 0;
}
char *
strchrnul(const char *s, int c_in)
{
	char c = c_in;
	while (*s && (*s != c))
		s++;
	
	return (char *) s;
}
#endif
