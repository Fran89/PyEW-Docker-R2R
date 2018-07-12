// mfc_dlog_app_base.cpp : Base class for MFC applications that use a dialog-style.
//

//#include <stdafx.h>

#include <mfc_dlog_app_base.h>
#include <comfile.h>

//#ifdef _DEBUG
//#define new DEBUG_NEW
//#undef THIS_FILE
//#define THIS_FILE "__FILE__"
//static char THIS_FILE[] = __FILE__;
//#endif



/////////////////////////////////////////////////////////////////////////////


/*
** This function is used by CMFCDialogAppBase, and all derivatives,
** to start worker threads running in a given class method.
**
** First, set class variable(s) to track start directives,
** then call this method, with 'this' as the parameter,
** afterwards the worker thread can determine from the class
** variables what the thread activities should be performed.
**
** Note that the class variables are not thread-safe, so 
** multiple worker threads, the caller should impose a sleep
** after each AfxBeginThread() to allow the worker thread to
** read the class variables before they are changed.
*/
UINT AFX_CDECL StartMFCWorkerThread(LPVOID p_object)
{
   CMFCDialogAppBase * p_sr = (CMFCDialogAppBase *)p_object;

   p_sr->StartWorkerThread();

   return 0;
}


/////////////////////////////////////////////////////////////////////////////
// CMFCDialogAppBase

BEGIN_MESSAGE_MAP(CMFCDialogAppBase, CWinApp)
	//{{AFX_MSG_MAP(CMFCDialogAppBase)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
   // Using dialog-based application needs special idle processing
   // to updates menus, ets.
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMFCDialogAppBase construction

CMFCDialogAppBase::CMFCDialogAppBase()
{
   LoggingLevel = 0;
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CMFCDialogAppBase object

// CMFCDialogAppBase theApp;


/////////////////////////////////////////////////////////////////////////////
// CMFCDialogAppBase initialization

BOOL CMFCDialogAppBase::InitInstance()
{

   try
   {
#ifdef _DEBUG
      TLogger::TruncateOnOpen();
#endif

      // m_lpCmdLine should be the configuration (.d) file name

      // Check the command line
      if ( m_lpCmdLine[0] == '\0' )
      {
         throw worm_exception("missing command line parm 1 (configuration file name)");
      }


      if ( ! PrepApp(m_lpCmdLine) )
      {
         throw worm_exception("failed preparing application");
      }


      if ( ! ParseCommandFile(m_lpCmdLine) )
      {
         // error parsing command file
         worm_exception _expt("Error parsing command file ");
                        _expt += m_lpCmdLine;
         throw _expt;
      }

      // Running = true so any threads started in InitApp()
      //           won't quit right away
      Running = true;

      if ( ! InitApp() )
      {
         throw worm_exception("Failed call to InitApp()");
      }


	   m_pMainWnd = GetMainWindow();

      OpenMainDialog();

   }
   catch( worm_exception _we )
   {
      TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TIMESTAMP
                   , "%s: Exiting due to\n%s\n"
                   , GetApplicationName()
                   , _we.what()
                   );
   }


   TLogger::Close();


	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////


bool CMFCDialogAppBase::ParseCommandFile( LPTSTR p_filename )
{
   bool r_status = true;
   
   TComFileParser * _parser = NULL;

   try
   {

      if ( (_parser = new TComFileParser()) == NULL )
      {
         throw worm_exception("failed creating TComFileParser to parse configuration file");
      }

      if ( ! _parser->Open(p_filename) )
      {
         char _msg[80];
         sprintf( _msg, "failed opening configuration file %s", p_filename );
         throw worm_exception(_msg);
      }


      bool _reading = true;

      char * _token;

      do
      {
          switch( _parser->ReadLine() )
          {
            case COMFILE_EOF:  // eof
                 _reading = false;
                 break;

            case COMFILE_ERROR: // error
                 throw worm_exception("error returned by TComFileParser::ReadLine()");

            case 0:  // empty line, TComFileParser should not return this
                 break;

            default:

                 // Get the command into _token
                 _token = _parser->NextToken();

                 // check if module .d commands handled by deriving class
                 //
                 if ( HandleConfigLine(_parser) != HANDLER_UNUSED )
                 {
                    continue;
                 }

                 // Not handled 
                 //
                 // Don't throw an error here because it is preferable to report
                 // as many errors as possible to the file.
                 //
                 TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TIMESTAMP
                               , "ParseCommandFile(): unrecognized command: %s\n"
                               , _token
                               );
                    
                 r_status = false;
          }
      } while( _reading );

      delete( _parser );
      _parser = NULL;


      // Is derivative class ready?
      //
      if ( ! IsReady() )
      {
         throw worm_exception("application not configured properly");
      }

   }
   catch( worm_exception _we )
   {
      TLogger::Logit( WORM_LOG_TOFILE
                   , "ParseCommandFile(): Error: \n%s\n"
                   , _we.what()
                   );
      r_status = false;
   }
   
   if ( _parser != NULL )
   {
      delete( _parser ) ;
   }


   
   return r_status;
}

/////////////////////////////////////////////////////////////////////////////


