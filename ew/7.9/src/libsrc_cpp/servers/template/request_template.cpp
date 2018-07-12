/*
 * request_template.cpp -- Example of a request message class for use
 *                         in the mutable server model.
 *
 *                       In addition to passport handling, this example
 *                       class is expected to also handle an integer and
 *                       a char string of undetermined length (to demonstrate
 *                       memory allocation and cleanup).
 */

// START WITH THE CLASS DECLARATION IN THE .h FILE.

//******************************************************************************
// STEP 1: Do a global replacement of the string "RequestTemplate"
//         with the name of the new class
//******************************************************************************

 
//******************************************************************************
// STEP 2: Replace the name of the include file for the class declaration
//
//******************************************************************************
#include "request_template.h"


// THIS INCLUDE IS USED FOR THIS EXAMPLE, AND MAY NOT BE NEEDED FOR REAL
// IMPLEMENTATIONS.
#include <string.h>

//******************************************************************************
// STEP 3: Update the constructor to initialize all variables that were
//         declared in the .h (Base classes will initialize their own 
//         variables as needed).
//
//******************************************************************************
//------------------------------------------------------------------------------
RequestTemplate::RequestTemplate()
{
   // Any pointers that will reference memory allocated by this class
   // MUST be NULLified here.
   MyInt = 0;
   MyCharPointer = NULL; // allocated in SetMyString()
   MyStringLength = -1;  // indicates not even end-of-string  
}

//******************************************************************************
// STEP 4: If the destructor declaration was included in the .h file,
//         implement it here, otherwise remove this method.
//
//******************************************************************************
//------------------------------------------------------------------------------
RequestTemplate::~RequestTemplate()
{
   if ( MyCharPointer != NULL )
   {
      delete [] MyCharPointer;
   }
   // The class can't be used after destruction, so there is no need
   // to set MyCharPointer back to NULL.
}

//******************************************************************************
// STEP 5: Implement the accessor methods declared for this specific class:
//         (there are four of them for this example)
//
//******************************************************************************

//------------------------------------------------------------------------------
void RequestTemplate::SetMyInteger( int p_int ) { MyInt = p_int; }
//------------------------------------------------------------------------------
bool RequestTemplate::SetMyString( const char * p_str )
{
   bool return_status = true;  // assume good, unless error encountered
   
   if (   p_str == NULL
       || MyStringLength < strlen(p_str)
      )
   {
      // Attempting to set the string to NULL
      // Or the buffer is not large enough to contain it
      
      // Drop any unrequired memory
      
      if ( MyCharPointer != NULL )
      {
         delete [] MyCharPointer;
         
         // MUST set the pointer to NULL to prevent a second
         // attempt to deallocate when the destructor is called.
         MyCharPointer = NULL;
      }
      
      MyStringLength = -1;
   }
   
   
   if ( p_str != NULL )
   {
      // Attempting to set the string to something
      //
     
      // [re]-allocate the buffer if needed
      
      if ( MyCharPointer == NULL )
      {
         MyStringLength = strlen(p_str);
         
         if ( (MyCharPointer = new char[ MyStringLength + 1 ]) == NULL )
         {
            // failed to get buffer memory
            return_status = false;
         }
      }
      
      if ( MyCharPointer != NULL )
      {
         // Have a buffer, assume it is of sufficient length.
         // Copy the string into the memory
         
         strcpy( MyCharPointer, p_str );
      }
   }
   
   return return_status;
}
//------------------------------------------------------------------------------
int RequestTemplate::GetMyInteger() const { return MyInt; }
//------------------------------------------------------------------------------
char * RequestTemplate::GetMyString() const { return MyCharPointer; }

//******************************************************************************
// STEP 6: Implement the virtual methods that were declared but not defined
//         in the Base classes from which this one is derived.
//         (Deriving from MutableServerRequest means there are three, there may
//         be a different number for a different base class).
//
//******************************************************************************

//------------------------------------------------------------------------------
long RequestTemplate::BufferInitAlloc()
{
   long ExpectedBufferLength = 0;
   
   // First, call the immediate base class's method to get the buffer length
   // required by all base classes.
   // 
   // THIS CALL MUST BE INCLUDED IN THIS METHOD,
   // AND IT MUST BE CALLED FOR THE CLASS FROM WHICH THIS ONE IS DERIVED.
   //
   ExpectedBufferLength = MutableServerRequest::BufferInitAlloc();
  
  
   // NOW, ADD THE ADDITIONAL BUFFER LENGTH NEEDED BY THIS CLASS
   //
   // FOR SIMPLE MESSAGE CLASSES, THIS METHOD MIGHT BE SIMPLER.
   // THE MAIN PURPOSE OF THE METHOD IS HELP REDUCE MEMORY REALLOCATIONS
   // WHEN THE FINAL LENGTH OF THE MESSAGE IS UNKNOWN AT CODE TIME.
   // FOR EXAMPLE, FOR MESSAGES THAT MIGHT HAVE ANY NUMBER OF DATA VALUES
   // (i.e. like a message with zero or n passport lines).
   //
   
   
   // For this example, assume that MyInt won't be more than 15 digits:
   
   ExpectedBufferLength += 15;
   
   // Presuming that the int and string values for this class will be
   // separated by '\n', add the length needed for that single char:
   
   ExpectedBufferLength += 1;
   
   // Add then length of the string, if the string is not NULL
   
   if ( MyCharPointer != NULL )
   {
      ExpectedBufferLength += strlen( MyCharPointer );
   }

   // For this example, assume that we will always have an empty line
   // even if the string contents are NULL
   ExpectedBufferLength += 1;  // '\n'

   
   return ExpectedBufferLength;
}
//------------------------------------------------------------------------------
void RequestTemplate::FormatDerivativeData()
{
   // First, prepare the leading part of the buffer....that is, prepare
   // the buffer part used by the base class(es):
   //
   // THIS CALL MUST BE PERFORMED AT THE START OF THIS METHOD,
   // AND IT MUST BE CALLED FOR THE CLASS FROM WHICH THIS ONE IS DERIVED.
   //
   MutableServerRequest::FormatDerivativeData();
   
   // NOW FORMAT AND APPEND THE BUFFER CONTENTS FOR THIS CLASS.
   
   // For this example, we've got an int and possibly a char string.
   // Each of which will be on a "line" terminated with '\n'
   
   // THE ACTUAL BUFFER IS IMPLEMENTED WITH A C++ basic_string TYPE.
   // A CHAR VALUE MAY BE APPENDED TO THE BUFFER WITH += AS SHOWN:
   
   char _myint[18];  
   
   // format the MyInt line (including the line terminator)
   //
   sprintf( _myint, "%d\n MyInt );
   
   MessageBuffer += _myint;

   // Add the string, if it is not null
   
   if ( MyCharPointer != NULL )
   {
      MessageBuffer += MyCharPointer;
   }
   
   // Terminate the string line
   
   MessageBuffer += "\n";
   
   // THE BASE CLASS PROCESSING WILL TERMINATE THE ENTIRE MESSAGE
   // WITH AN ADDITIONAL '\N'
   
}
//------------------------------------------------------------------------------
void RequestTemplate::ParseDerivativeData()
{
   // First, parse the leading part of the buffer....that is,
   // the buffer part used by the base class(es):
   //
   // THIS CALL MUST BE PERFORMED AT THE START OF THIS METHOD,
   // AND IT MUST BE CALLED FOR THE CLASS FROM WHICH THIS ONE IS DERIVED.
   //
   MutableServerRequest::ParseDerivativeData();

   long _index;            // end of line index
   std::string _readline;  // work buffer
   
   // NOW PARSE THE PORTION OF THE BUFFER RELEVANT TO THIS MESSAGE CLASS

   // NOTE THAT ALL ERRORS ARE REPORTED BY THROWING A worm_exception OBJECT.
   // THERE ARE TWO WAYS TO FORMAT THE ERROR MESSAGE CONTAINED THEREIN, BOTH
   // ARE DEMONSTRATE HEREIN.
   // 
   // FYI, THE worm_exception IS CAUGHT AND REPORTED BY THE SERVER

   
   // - - - - - - - - - - - - - - - - - - - - - - - - - -
   // Handle the example's first data line


   // Find the end of the next line in the buffer
   //
   if ( (_index = MessageBuffer.find("\n")) == MessageBuffer.npos )
   {
      // First way to throw a worm_exception object
      throw worm_exception("Unterminated message while parsing RequestTemplate MyInt line");
   }   
   
   // extract the line from the buffer
   //
   _readline = MessageBuffer.substr( 0 , _index );
   

   // Remove the line from the buffer
   //
   MessageBuffer.erase( 0 , _index + 1 );
   
   
   // Parse the line's value  (MyInt)
   //
   if ( sscanf( _readline.c_str(), "%d", &MyInt ) != 1 )
   {
      // Second way to throw a worm_exception object -- append string (int, float) value
      worm_exception my_exception("RequestTemplate::ParseDerivativeData() Error: ");
                     my_exception += "Failed to parse MyInt value";
      throw my_exception;
   }
   
   
   // - - - - - - - - - - - - - - - - - - - - - - - - - -
   // Handle the example's second data line
   //    (the string value [which may be empty])
   
   // Find then end of the next line in the buffer
   //
   if ( (_index = MessageBuffer.find("\n")) == MessageBuffer.npos )
   {
      throw worm_exception("Unterminated message while parsing RequestTemplate MyString line");
   }

   // extract the line from the buffer
   //
   _readline = MessageBuffer.substr( 0 , _index );

   // Remove the line from the buffer
   //
   MessageBuffer.erase( 0 , _index + 1 );

   // Set the string value
   //
   if ( _readline.size() == 0 )
   {
      // empty line      
      SetMyString( NULL );
   }
   else
   {
      // something in the line
      SetMyString( _readline.c_str() );
   }
   
   // Any remaining buffer (resulting from further class derivation or
   // just the end-of-message '\n') parse elsewhere
}
//------------------------------------------------------------------------------
