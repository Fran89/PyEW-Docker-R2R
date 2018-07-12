/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: itemlist.c 44 2000-03-13 23:49:34Z lombard $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/03/13 23:45:14  lombard
 *     Initial revision
 *
 *
 *
 */

#include <dcc_std.h>
#include <dcc_misc.h>

PRIVATE VOID	_CreateItem(char *item, ITEMLIST **root, ITEMLIST **tail)
{

	char	*itemcopy;
	ITEMLIST *newitem;

	if (item) 	itemcopy = SafeAllocString(item);
	else		itemcopy = NULL;

	/*printf("_CreateItem(%s)\n",itemcopy==NULL?"NULL":itemcopy);*/

	newitem = SafeAlloc(sizeof(ITEMLIST));

	newitem->next = NULL;
	newitem->item = itemcopy;

	if (*root==NULL)	*root = newitem;
	else (*tail)->next = newitem;
	*tail = newitem;

}

_SUB ITEMLIST *MakeItemList(char *inputstring, char separator,
	BOOL killwhitespace, BOOL permitnullitems)
{

	ITEMLIST *root=NULL, *tail=NULL;
	char sambuf[512],*inputptr,*outputptr;
	int bufpop;

	bufpop = 0;
	outputptr = sambuf;

	/*printf("MakeItemList(%s,%c,%d,%d)\n",
		inputstring,separator,killwhitespace,permitnullitems);*/

	for (inputptr = inputstring; *inputptr!='\0'; inputptr++) {

		if (*inputptr==separator) {
			if (bufpop==0) {
				if (permitnullitems)
					_CreateItem(NULL,&root,&tail);
				continue;
			}
			*outputptr = '\0';
			_CreateItem(sambuf,&root,&tail);
			bufpop=0;
			outputptr = sambuf;
			continue;
		}
		
		if (killwhitespace&&isspace(*inputptr)) continue;	

		*outputptr++ = *inputptr;
		bufpop++;

	}

	if (bufpop>0) {
		*outputptr = '\0';
		_CreateItem(sambuf,&root,&tail);
	}

	return(root);
}

_SUB 	VOID  FreeItemList(ITEMLIST *root)
{

	ITEMLIST *next,*loop;

	for (loop=root; loop!=NULL; loop=next) {
		next = loop->next;
		SafeFree(loop->item);
		SafeFree(loop);
	}
}

_SUB	VOID	AppendItemList(ITEMLIST *originallist, ITEMLIST *newlist)
{

	ITEMLIST *loop;

	if (originallist==NULL) return;
	for (loop=originallist; loop->next!=NULL; loop=loop->next);

	loop->next = newlist;

}
