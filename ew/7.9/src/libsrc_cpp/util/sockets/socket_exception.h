//---------------------------------------------------------------------------
#ifndef socket_exceptionH
#define socket_exceptionH
//---------------------------------------------------------------------------
#include <worm_socket.h>
#include <worm_exceptions.h>


//---------------------------------------------------------------------------

class worm_socket_exception : public worm_exception
{
protected:
   WS_FUNCTION_ID FunctionId;
   int ErrorCode;
   int CloseType;
public:
   worm_socket_exception( WS_FUNCTION_ID p_functionid
                        , int p_errorcode
                        , const char * p_what
                        , int p_closesocket = 0
                        ) : worm_exception( p_what )
   {
      FunctionId = p_functionid;
      ErrorCode = p_errorcode;
      CloseType = p_closesocket;
   }
   worm_socket_exception( WS_FUNCTION_ID p_functionid
                        , int p_errorcode
                        , std::string p_what
                        , int p_closesocket = 0
                        ) : worm_exception( p_what )
   {
      FunctionId = p_functionid;
      ErrorCode = p_errorcode;
      CloseType = p_closesocket;
   }

   static const char * DecodeError( WS_FUNCTION_ID p_functionid, int p_errcode );

   const char * DecodeError() const;
   const WS_FUNCTION_ID GetFunctionId() { return FunctionId; }
   const int GetErrorCode() { return ErrorCode; }
   const int SocketCloseType() { return CloseType; }
};


#endif
 