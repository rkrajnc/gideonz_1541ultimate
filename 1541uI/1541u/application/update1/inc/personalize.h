#ifndef PERSONALIZE_H
#define PERSONALIZE_H

#define NUM_FIELDS 17

struct user_info
{
    char *units;
    char *type;
    char *handle;
    char *email;
    char *firstname;
    char *lastname;
    char *fullname;
    char *address1;
    char *address2;
    char *address3;
    char *address4;
    char *method;
    char *amount;
    char *date;
    char *status;
    char *payment;
    char *ar_rom;
};

BOOL personalize(struct user_info *);

WORD read_index(void);
BOOL write_index(WORD index);
int read_line(FIL *my_file, char *line, int maxlen);
struct user_info *decode_user(char *line);
struct user_info *get_user(WORD index);
BOOL log_serial(BYTE *serial, struct user_info *user);
BOOL store_name_in_ee(char *name);

#endif
