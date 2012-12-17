#ifndef PERSONALIZE_H
#define PERSONALIZE_H

#define NUM_FIELDS 24

struct user_info
{
    char *units;      // Units          
    char *type;       // Type        
    char *batch;      // Batch       
    char *handle;     // Handle      
    char *email;      // E-mail      
    char *firstname;  // First Name  
    char *lastname;   // Last Name   
    char *fullname;   // Full Name   
    char *address1;   // Address 1   
    char *address2;   // Address 2   
    char *address3;   // Address 3   
    char *address4;   // Address 4   
    char *region;     // Region
    char *method;     // Method      
    char *amount;     // Amount      
    char *date;       // Date        
    char *orderdate;  // OrderDate
    char *paymentdate;// PaymentDate
    char *shippingdate; // SD
    char *status;     // Status      
    char *payment;    // Payment     
    char *remarks;    // Remarks     
    char *id;         // ID
    char *invoice;    // Invoice          
};

BOOL personalize(struct user_info *);

WORD read_index(void);
BOOL write_index(WORD index);
int read_line(FIL *my_file, char *line, int maxlen);
struct user_info *decode_user(char *line);
struct user_info *get_user(WORD index);
BOOL log_serial(BYTE *serial, struct user_info *user);
BOOL log_status(struct user_info *user);
BOOL store_name_in_ee(char *name);

#endif
