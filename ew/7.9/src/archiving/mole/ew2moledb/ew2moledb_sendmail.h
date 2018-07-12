#ifndef EW2MOLEDB_SENDMAIL_H
#define EW2MOLEDB_SENDMAIL_H 1

#define MAX_SIZE_STRING_SENDMAIL 512

#define EW2MOLEDB_MAXRECIP     10
#define EW2MOLEDB_MAXRECIPLEN  60
extern int  ew2moledb_nmailRecipients;
extern char ew2moledb_mailRecipients[EW2MOLEDB_MAXRECIP][EW2MOLEDB_MAXRECIPLEN];
extern char ew2moledb_mailProg[MAX_SIZE_STRING_SENDMAIL];
extern char ew2moledb_mailServer[MAX_SIZE_STRING_SENDMAIL];
extern char ew2moledb_mailFrom[MAX_SIZE_STRING_SENDMAIL];

int ew2moledb_SendMail(char *subject, char *msg);

#endif

