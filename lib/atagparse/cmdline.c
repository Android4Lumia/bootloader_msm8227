#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <list.h>

typedef struct cmdline_item {
	struct list_node node;
	char* name;
	char* value;
} cmdline_item_t;

static struct list_node cmdline_list;

static cmdline_item_t* cmdline_get_internal(const char* name) {
	cmdline_item_t *item;
	list_for_every_entry(&cmdline_list, item, cmdline_item_t, node) {
		if(!strcmp(name, item->name))
			return item;
	}

	return NULL;
}

bool cmdline_has(const char* name) {
	return !!cmdline_get_internal(name);
}

const char* cmdline_get(const char* name) {
	cmdline_item_t* item = cmdline_get_internal(name);

	if(!item)
		return NULL;

	return item->value;
}

void cmdline_add(const char* name, const char* value, bool overwrite) {
	cmdline_item_t* item = cmdline_get_internal(name);
	if(item) {
		if(!overwrite) return;

		list_delete(&item->node);
		free(item->name);
		free(item->value);
		free(item);
	}

	item = malloc(sizeof(cmdline_item_t));
	item->name = strdup(name);
	item->value = value?strdup(value):NULL;

	list_add_tail(&cmdline_list, &item->node);
}

void cmdline_remove(const char* name) {
	cmdline_item_t* item = cmdline_get_internal(name);
	if(item) {
		list_delete(&item->node);
		free(item->name);
		free(item->value);
		free(item);
	}
}

size_t cmdline_length(void) {
	size_t len = 0;

	cmdline_item_t *item;
	list_for_every_entry(&cmdline_list, item, cmdline_item_t, node) {
		if(len!=0) len++;
		len+=strlen(item->name);
		if(item->value)
			len+= 1 + strlen(item->value);
	}

	// 0 terminator
	len++;

	return len;
}

size_t cmdline_generate(char* buf, size_t bufsize) {
	size_t len = 0;

	cmdline_item_t *item;
	list_for_every_entry(&cmdline_list, item, cmdline_item_t, node) {
		if(len!=0) buf[len++] = ' ';
		len+=strlcpy(buf+len, item->name, bufsize-len);

		if(item->value) {
			buf[len++] = '=';
			len+=strlcpy(buf+len, item->value, bufsize-len);
		}
	}

	return len;
}

static int str2nameval(const char* str, char** name, char** value) {
	char *c;
	int index;
	char* ret_name;
	char* ret_value;

	// get index of delimiter
	c = strchr(str, '=');
	if(c==NULL) {
		*name = strdup(str);
		*value = NULL;
		return -1;
	}
	index = (int)(c - str);

	// get name
	ret_name = malloc(index+1);
	memcpy(ret_name, str, index);
	ret_name[index] = 0;

	// get value
	ret_value = strdup(str+index+1);

	*name = ret_name;
	*value = ret_value;

	return 0;
}

void cmdline_addall(char* cmdline, bool overwrite) {
	char* sep = " ";

	char *pch = strtok(cmdline, sep);
	while (pch != NULL) {
		char* name = NULL;
		char* value = NULL;
		str2nameval(pch, &name, &value);

		cmdline_add(name, value, overwrite);
		free(name);
		if(value) free(value);

		pch = strtok(NULL, sep);
	}
}

void cmdline_init(void)
{
	list_initialize(&cmdline_list);
}
