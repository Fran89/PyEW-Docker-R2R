/*
 * result_template.cpp -- Template of a result message class for use
 *                        in the mutable server model.
 * 
 * See request_template.cpp for example code and comments
 * or magserverresult.cpp for a detailed implementation example.
 */

// START WITH THE CLASS DECLARATION IN THE .h FILE.

//******************************************************************************
// STEP 1: Do a global replacement of the string "ResultTemplate"
//         with the name of the new class
//******************************************************************************

 
//******************************************************************************
// STEP 2: Replace the name of the include file for the class declaration
//
//******************************************************************************
#include "result_template.h"



//******************************************************************************
// STEP 3: Update the constructor to initialize all variables that were
//         declared in the .h (Base classes will initialize their own 
//         variables as needed).
//
//******************************************************************************
//------------------------------------------------------------------------------
ResultTemplate::ResultTemplate()
{
   // INITIALIZE ANY CLASS VARIABLES
   
   // Any pointers that will reference memory allocated by this class
   // MUST be NULLified here.
   
}

//******************************************************************************
// STEP 4: If the destructor declaration was included in the .h file,
//         uncomment and implement it here, otherwise remove this method.
//
//******************************************************************************
//------------------------------------------------------------------------------
/*
ResultTemplate::~ResultTemplate()
{
   // MEMORY ALLOCATED WITHIN THIS CLASS MUST BE DELETED HERE
   // (IF NOT PREVIOUSLY)
    
   // EXAMPLE
   // if ( MyCharPointer != NULL )
   // {
   //   delete [] MyCharPointer;
   // }
   // The class can't be used after destruction, so there is no need
   // to set MyCharPointer back to NULL.
}
*/

//******************************************************************************
// STEP 5: Implement the accessor methods declared for this specific class:
//         (these examples equate to those in the template .h file)
//
//******************************************************************************

//------------------------------------------------------------------------------
/*
void ResultTemplate::SetMyInt( int parameterA )
{
   MyInt = parameterA;
}
*/
//------------------------------------------------------------------------------
/*
int ResultTemplate::GetMyInt() { return MyInt; }
*/


//******************************************************************************
// STEP 6: Implement the virtual methods that were declared but not defined
//         in the Base classes from which this one is derived.
//         (Deriving from MutableServerResult means there are three, there may
//         be a different number for a different base class).
//
//******************************************************************************


//------------------------------------------------------------------------------
long ResultTemplate::BufferInitAlloc()
{
   long ExpectedBufferLength = 0;
   
   // First, call the immediate base class's method to get the buffer length
   // required by all base classes.
   // 
   // THIS CALL MUST BE INCLUDED IN THIS METHOD,
   // AND IT MUST BE CALLED FOR THE CLASS FROM WHICH THIS ONE IS DERIVED.
   //
   ExpectedBufferLength = MutableServerResult::BufferInitAlloc();
  
   // ADD THE ADDITIONAL BUFFER LENGTH NEEDED BY THIS CLASS'S VARIABLES
   //
   // Assuming a single int value which will never be more than 15 digits,
   // it might be as simple as the line below.  For more complex results
   // (for example magnitude results that might have n channels included),
   // it could be much more complex.
   
   ExpectedBufferLength += 15;
   
   return ExpectedBufferLength;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void ResultTemplate::FormatDerivativeData()
{
   // First, prepare the leading part of the buffer....that is, prepare
   // the buffer part used by the base class(es):
   //
   // THIS CALL MUST BE PERFORMED AT THE START OF THIS METHOD,
   // AND IT MUST BE CALLED FOR THE CLASS FROM WHICH THIS ONE IS DERIVED.
   //
   MutableServerResult::FormatDerivativeData();
   
   
   // NOW FORMAT AND APPEND THE BUFFER CONTENTS FOR THIS CLASS.
   
   // THE ACTUAL BUFFER IS IMPLEMENTED WITH A C++ basic_string TYPE.
   // A CHAR VALUE MAY BE APPENDED TO THE BUFFER WITH +=
   //
   // MessageBuffer += "SOME STRING VALUE";

   
   // GENERALLY, THIS CLASS HEIRARCHY BREAKS MESSAGES INTO MULTIPLE LINES
   // WITH '\n'.  THE END OF THE MESSAGE IS MARKED WITH AN ADDITIONAL '\n'.
   
   
   /*
   // Say that this class has some result value of type int, the code to
   // include that value in the message buffer might look something like:

   char _myint[18];  
   
   sprintf( _myint, "%d\n MyInt );
   
   MessageBuffer += _myint;
   */
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void ResultTemplate::ParseDerivativeData()
{
   // One of the first things done as part of the parse process is that the
   // message is put into the (base class's) variable MessageBuffer
   // 
   // Lines are then snipped off the front of the buffer as they are processed.
   //
   
   // First, parse the leading part of the buffer....that is,
   // the buffer part used by the base class(es):
   //
   // THIS CALL MUST BE PERFORMED AT THE START OF THIS METHOD,
   // AND IT MUST BE CALLED FOR THE CLASS FROM WHICH THIS ONE IS DERIVED.
   //
   MutableServerResult::ParseDerivativeData();


   long _index;            // end of line index
   std::string _readline;  // work buffer
   
   
   // NOW PARSE THE PORTION OF THE BUFFER RELEVANT TO THIS MESSAGE CLASS

   // NOTE THAT ALL ERRORS ARE REPORTED BY THROWING A worm_exception OBJECT.
   // THERE ARE TWO WAYS TO FORMAT THE ERROR MESSAGE CONTAINED THEREIN, EITHER
   // WITH A SIMPLE STRING:
   //
   //    throw worm_exception("My error message");
   //
   // OR BUILT UP FROM STRINGS AND VARAIBLES
   //
   //    worm_exception my_exception("ResultTemplate::ParseDerivativeData() Error: ");
   //                   my_exception += SomeStringVariable;
   //                   my_exception += " ";
   //                   my_exception += SomeOtherStringVariable;
   //    throw my_exception;
   // 
   // FYI, THE worm_exception IS CAUGHT AND REPORTED BY THE SERVER

   
   // MESSAGE LINES ARE GENERALLY HANDLED WITH ONE OR MORE REPETITIONS OF
   // THE FOLLOWING FOUR STEPS.
   // THE MESSAGE IS TERMINATED WITH AN EMPTY LINE ('\n' ALONE),
   // THUS, EITHER HAVE A FIXED NUMBER OF LINES FOR THE CLASS, OR BE
   // PREPARED FOR MESSAGE TERMINATION.


   // 1. Find the end of the next line in the buffer
   //
   if ( (_index = MessageBuffer.find("\n")) == MessageBuffer.npos )
   {
      // First way to throw a worm_exception object
      throw worm_exception("Unterminated message while parsing ResultTemplate line");
   }   
   
   // 2. Extract the line from the buffer
   //
   _readline = MessageBuffer.substr( 0 , _index );
   
   // 3. Remove the line from the buffer
   //
   MessageBuffer.erase( 0 , _index + 1 );
   
   // 4. Parse the message line
   
   if ( sscanf( _readline.c_str()
              , "%d %d %s"
              , &MyIntVariable
              , &MySecondIntVariable
              ,  MyCharVariable
              ) != 3
      )
   {
      worm_exception my_exception("ResultTemplate::ParseDerivativeData() Error: Invalid line\n" );
                     my_exception += _readline.c_str();
      throw my_exception;
   }
   
   // NOTE THAT THE ABOVE 'MyXXXVariables' ARE EXPECTED TO BE CLASS VARIABLES,
   // THUS AFTER PARSING, THIS CLASS BECOMES A DATA CONTAINER ALONG WITH
   // MESSAGE FORMATTER/PARSER
   
}

//------------------------------------------------------------------------------

