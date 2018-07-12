I.    Introduction
II.   Inheritance
III.  Message Passing
IV.   Request Message Types
V.    Result Message Types
VI.   Creating A New Request Type
VII.  Creating A New Result Type
VIII. Deriving The New Server
IX.   Other Files


I. Introduction
---------------

This is a template to assist in the creation of C++ servers deriving
from the C++ mutable server base classes.

As background, the mutable server model represents a type of process
that can act as a stand-alone executable, an Earthworm module a server
that listens to TCP socket calls, or the client to such a server.

The base classes provided reused functionality such as configuration
file parsing, logging, ring and/or socket management; and at a higher
level, activities such as message parsing, etc.

The intention of this model is to allow developers to focus primarily
upon the unique funtionality of the server, not the plumbing.

The template consists of three classes in six files:
                               
   REQUEST MESSAGE:
   
       request_template.cpp
       request_template.h
                               
   RESULT MESSAGE:
   
       result_template.cpp
       result_template.h

   SERVER:

       server_template.cpp  -- the basic server implementation class,
                               with partially-completed functions
       server_template.h    -- the related header file
                               (wherein relevant variables are declared)
       
It is expected that the developer will copy these items into a separate
directory, renaming and completing as appropriate.


II. Inheritance
---------------

The most difficult aspect for C developers will be likely the concept of
inheritance.  That is the C++ construct whereby one class derives from
another, thereby obtaining the functionality implemented in the parent
(or "base") class.

In practice deriving a new class mostly means creating a header file to
contain any additional variables needed, and define any new functions
["methods"].  And creating a new source (.cpp) file to implement the
required methods (both new method, and those which were declared, but
not coded, in the base class).

All of the templates inherit from some base class, but they are commented,
and this description continues, with the intent to make all of this easy
as possible.


III. Message Passing
--------------------

To reduce code repetition, the mututable server model anticipates passing
information between some internal functions in the form of message classes
that contain relevant information and can also format that information
into ASCII strings appropriate for transmission via an Earthworm ring, or
across a socket.  (This facilitates the 'mutable' characteristic of the
server).

Generally, a mutable server will deal with both a request message and a
result message.  Depending on requirements, a given server may use an
existing message type, or derive a new type for either, both or neither.

At the top of the base message class heirarchy is the MutableServerMessage
class.  It provides a buffer area for formatting messages, and a set of
methods that act as an interface asserting that all messages must be
formattable and parseable.


IV. Request Message Types
-------------------------

The MutableServerRequest class derives directly from MutableServerMessage.
It provides additional functionality to handle Passport lines.

At the time of writing, there is a further derivation, MLServerRequest
(under /earthworm/Contrib/Golden/NEIS/../src/mlserver).  This class
provides the functionality to contain, parse, and format an Origin id,
Event Id, and other seismic-related information along with the passport
info handled by MutableServerRequest.  It serves as an additional
example beyond the one to follow.


V.    Result Message Types
--------------------------

The MutableServerResult class also derives directly from MutableServerMessage.
It adds only the functionality to handle an integer status code.  (In the NEIC
server model, processing programs return one of three states: Good, Fail (to
calculate result), or Error (system problem).

An additional class, MagServerResult, was created in support of servers that
are producing magnitude results.  It has additional functionality to 
handle magnitude and channel information.


VI.   Creating A New Request Type
---------------------------------

To make a new class the first steps would be to decide upon the name for the new class,
copy the files request_template.h and request_template.cpp into an appropriate directory,
rename the files, then do a replacement of the string "RequestTemplate" with the
name of the new class in both files.

Following that, any additional variables and functions (class methods) will be 
declared in the .h file, and the methods implemented in the .cpp file.

The template code is in the form of an example of such a class which, in addition
to passport handling, is expected to handle an int value and a char string of
undetermined length.  The code for the template/example is in request_template.h
and request_template.cpp.

Start editing the new request from the .h file, follow the "STEP n" lines
in the file for specific instructions.  Additional comments are contained
therein.

Update the methods defined in the .cpp file, again following the "STEP n" lines.



VII.  Creating A New Result Type
--------------------------------

The process of creating a new result message type mirrors that for new request message
type.  The examples are in result_template.h and result_template.cpp.  The primary
difference from the request_template are in the result variables and accessor methods
which will pertain to the result.  There are also difference in the message formatting
and parsing methods, but the principles are exactly the same as in request_template.h/cpp.

Since the comments and examples for a new result type would be essentially equivalent to
those for a request type, the template code will be without the example detail in the
request type code.

Start by deciding on the name of the new result type, copy the result_template.h and
result_template.cpp files into the appropriate directory.

Edit the new request's .h file, following the "STEP n" lines in the file for specific
instructions.  Additional comments are contained therein.

Update the methods defined in the request's .cpp file, again following the "STEP n" lines.

If further example is needed, review the request_template.h/cpp or the magserverresult.h/cpp
code.


VIII. Deriving The New Server
-----------------------------

Most likely, a new server will use database access, thus should be derived from the parent
DBMutableServer.  If not, then using MutableServerBase as the parent class is adequate.
The code in server_template.h and server_template.cpp uses the former.

Rather than attempting a detail description here, the points in the code which require
alteration have been annotated with the string "STEP n" (where "n" is a one-up number).
There are also additional details contained in comments, particularly at the top of
server_template.cpp.

Copy the two files into another location, and begin in server_template.h.


IX. Other Files
---------------

Included are an example main.cpp, in which two minor changes are needed.  Also provided
are starting points for a dependency file and a make file.  The later two will require
substantial changes -- depending on the specific server derivation.
       