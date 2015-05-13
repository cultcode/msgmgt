#ifndef OPERATEXML_H 
#define OPERATEXML_H 

#define OPTIONS_LEN 128
#define OPTION_LEN 2048

extern int ReadConfigXml(char * fn_xml, char *** opt);

extern int ParseOptions(int argc,char**argv);

#endif
