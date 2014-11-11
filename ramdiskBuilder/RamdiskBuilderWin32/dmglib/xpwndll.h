#if WIN32 

#define DLLEXPORT __declspec(dllexport)

#ifdef __cplusplus
extern "C" {
#endif

DLLEXPORT int _cdecl  xpwntool_enc_dec(char* srcName, char* destName, char* templateFileName, char* ivStr, char* keyStr);

DLLEXPORT char* _cdecl xpwntool_get_kbag(char* fileName);


#ifdef __cplusplus
}
#endif

#endif //WIN32