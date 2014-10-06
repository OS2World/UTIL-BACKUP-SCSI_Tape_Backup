/*****************************************************************************
 * $Id: mangle.c,v 1.3 1992/09/12 15:57:36 ak Exp $
 *****************************************************************************
 * $Log: mangle.c,v $
 * Revision 1.3  1992/09/12  15:57:36  ak
 * - Usenet patches for GNU TAR 1.10
 * - Bugfixes and patches of Kai Uwe Rommel:
 *         filename conversion for FAT
 *         EMX 0.8e
 *         -0..1 alias for a: b:
 *         -2..7 alias for +++TAPE$x
 *
 * Revision 1.2  1992/09/02  20:08:16  ak
 * Version AK200
 * - Tape access
 * - Quick file access
 * - OS/2 extended attributes
 * - Some OS/2 fixes
 * - Some fixes of Kai Uwe Rommel
 *
 * Revision 1.1.1.1  1992/09/02  19:21:59  ak
 * Original GNU Tar 1.10 with some filenames changed for FAT compatibility.
 *
 * Revision 1.1  1992/09/02  19:21:57  ak
 * Initial revision
 *
 *****************************************************************************/

static char *rcsid = "$Id: mangle.c,v 1.3 1992/09/12 15:57:36 ak Exp $";

/* Modified 9/2/91 to fix bug in write_mangle of doing strlen(ptr1) when ptr1
   is only null terminated if malloc happens to have been kind.  This involved
   adding the buffer_length operation to port.c as well, which see.  Also,
   I added a flush_buffer at the end of write_mangle to repair a memory leak.
    -Max Hailperin <max@nic.gac.edu> 9/2/91 */

/* I added inverse_find_mangled to map a mangled name into the normal one for
   the say of verifying archives with mangled names.  See diffarch.c.
    -Max Hailperin <max@nic.gac.edu> 8/1/91 */

#include <sys/types.h>
#include <sys/stat.h>
#include "tar.h"

#ifdef __STDC__
#define VOIDSTAR void *
#else
#define VOIDSTAR char *
#endif
extern VOIDSTAR ck_malloc();
extern VOIDSTAR init_buffer();
extern char *quote_copy_string();
extern char *get_buffer();
extern char *index();

extern union record *start_header();

extern struct stat hstat;		/* Stat struct corresponding */

struct mangled {
	struct mangled *next;
	int type;
	char mangled[NAMSIZ];
	char *linked_to;
	char normal[1];
};


/* Should use a hash table, etc. .  */
struct mangled *first_mangle;
int mangled_num = 0;

char *
find_mangled (name)
char *name;
{
	struct mangled *munge;

	for(munge=first_mangle;munge;munge=munge->next)
		if(!strcmp(name,munge->normal))
			return munge->mangled;
	return 0;
}


char *
inverse_find_mangled (name)
char *name;
{
	struct mangled *munge;

	for(munge=first_mangle;munge;munge=munge->next)
		if(!strcmp(name,munge->mangled))
			return munge->normal;
	return 0;
}

 
#ifdef S_IFLNK
void add_symlink_mangle(symlink, linkto, buffer)
char *symlink;
char *linkto;
char *buffer;
{
	struct mangled *munge,*kludge;

	munge=(struct mangled *)ck_malloc(sizeof(struct mangled)+strlen(symlink)+strlen(linkto)+2);
	if(!first_mangle)
		first_mangle=munge;
	else {
		for(kludge=first_mangle;kludge->next;kludge=kludge->next)
			;
		kludge->next=munge;
	}
	munge->type=1;
	munge->next=0;
	strcpy(munge->normal,symlink);
	munge->linked_to=munge->normal+strlen(symlink)+1;
	strcpy(munge->linked_to,linkto);
	sprintf(munge->mangled,"@@MaNgLeD.%d",mangled_num++);
	strncpy(buffer,munge->mangled,NAMSIZ);
}
#endif

void
add_mangle (name, buffer)
char *name;
char *buffer;
{
	struct mangled *munge,*kludge;

	munge=(struct mangled *)ck_malloc(sizeof(struct mangled)+strlen(name));
	if(!first_mangle)
		first_mangle=munge;
	else {
		for(kludge=first_mangle;kludge->next;kludge=kludge->next)
			;
		kludge->next=munge;
	}
	munge->next=0;
	munge->type=0;
	strcpy(munge->normal,name);
	sprintf(munge->mangled,"@@MaNgLeD.%d",mangled_num++);
	strncpy(buffer,munge->mangled,NAMSIZ);
}

void
write_mangled()
{
	struct mangled *munge;
	struct stat hstat;
	union record *header;
	char *ptr1,*ptr2;
	VOIDSTAR the_buffer;
	int size;
	int bufsize;

	if(!first_mangle)
		return;
	the_buffer=init_buffer();
	for(munge=first_mangle,size=0;munge;munge=munge->next) {
		ptr1=quote_copy_string(munge->normal);
		if(!ptr1)
			ptr1=munge->normal;
		if(munge->type) {
			add_buffer(the_buffer,"Symlink ",8);
			add_buffer(the_buffer,ptr1,strlen(ptr1));
			add_buffer(the_buffer," to ",4);
			
			if(ptr2=quote_copy_string(munge->linked_to)) {
				add_buffer(the_buffer,ptr2,strlen(ptr2));
				free(ptr2);
			} else
				add_buffer(the_buffer,munge->linked_to,strlen(munge->linked_to));
		} else {
			add_buffer(the_buffer,"Rename ",7);
			add_buffer(the_buffer,munge->mangled,strlen(munge->mangled));
			add_buffer(the_buffer," to ",4);
			add_buffer(the_buffer,ptr1,strlen(ptr1));
		}
		add_buffer(the_buffer,"\n",1);
		if(ptr1!=munge->normal)
			free(ptr1);
	}

	bzero(&hstat,sizeof(struct stat));
	hstat.st_atime=hstat.st_mtime=hstat.st_ctime=time(0);
	ptr1=get_buffer(the_buffer);
	hstat.st_size=buffer_length(the_buffer);

	header=start_header("././@MaNgLeD_NaMeS",&hstat);
	header->header.linkflag=LF_NAMES;
	finish_header(header);
	size=hstat.st_size;
	header=findrec();
	bufsize = endofrecs()->charptr - header->charptr;

	while(bufsize<size) {
		bcopy(ptr1,header->charptr,bufsize);
		ptr1+=bufsize;
		size-=bufsize;
		userec(header+(bufsize-1)/RECORDSIZE);
		header=findrec();
		bufsize = endofrecs()->charptr - header->charptr;
	}
	bcopy(ptr1,header->charptr,size);
	bzero(header->charptr+size,bufsize-size);
	userec(header+(size-1)/RECORDSIZE);
        flush_buffer(the_buffer);
}

void
extract_mangle(head)
union record *head;
{
	char *buf;
	char *fromtape;
	char *to;
	char *ptr,*ptrend;
	char *nam1,*nam1end;
	int size;
	int copied;

	size=hstat.st_size;
	buf=to=ck_malloc(size+1);
	buf[size]='\0';
	while(size>0) {
		fromtape=findrec()->charptr;
		if(fromtape==0) {
			msg("Unexpected EOF in mangled names!");
			return;
		}
		copied=endofrecs()->charptr-fromtape;
		if(copied>size)
			copied=size;
		bcopy(fromtape,to,copied);
		to+=copied;
		size-=copied;
		userec((union record *)(fromtape+copied-1));
	}
	for(ptr=buf;*ptr;ptr=ptrend) {
		ptrend=index(ptr,'\n');
		*ptrend++='\0';

		if(!strncmp(ptr,"Rename ",7)) {
			nam1=ptr+7;
			nam1end=index(nam1,' ');
			while(strncmp(nam1end," to ",4)) {
				nam1end++;
				nam1end=index(nam1end,' ');
			}
			*nam1end='\0';
			if(ptrend[-2]=='/')
				ptrend[-2]='\0';
			un_quote_string(nam1end+4);
			if(rename(nam1,nam1end+4))
				msg_perror("Can't rename %s to %s",nam1,nam1end+4);
			else if(f_verbose)
				msg("Renamed %s to %s",nam1,nam1end+4);
		}
#ifdef S_IFLNK
		else if(!strncmp(ptr,"Symlink ",8)) {
			nam1=ptr+8;
			nam1end=index(nam1,' ');
			while(strncmp(nam1end," to ",4)) {
				nam1end++;
				nam1end=index(nam1end,' ');
			}
			un_quote_string(nam1);
			un_quote_string(nam1end+4);
			if(symlink(nam1,nam1end+4) && (unlink(nam1end+4) || symlink(nam1,nam1end+4)))
				msg_perror("Can't symlink %s to %s",nam1,nam1end+4);
			else if(f_verbose)
				msg("Symlinkd %s to %s",nam1,nam1end+4);
		}
#endif
		else
			msg("Unknown demangling command %s",ptr);
	}
}
