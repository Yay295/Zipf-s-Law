/**************************************************************************//**
@file

@mainpage Program 3: Zipf's Law

@section course_section Course Information

@author John Colton

@date Monday November 17, 2015

@par Professor: Paul Hinker

@par Course: CSC 300 - Fall 2015

@section program_section Program Information

@details This program counts the number of occurences of every word in a given
		 file and outputs this information to <filename>.wrd and <filename>.csv
		 in appropriate formats for those file types.

@section compile_section Compiling and Usage

@par Compiling Instructions:
	All:
		0. Go to dictionary.h and set the defines to fit your needs.
	MSVC++:
		1. Create a new program.
		2. Add Main.cpp and dictionary.cpp to your source files.
		3. Add dictionary.h to your header files.
		4. Compile
	GCC:
		1. Place Main.cpp, dictionary.h, and dictionary.cpp in a directory.
		2. CD to that directory.
		3. Run the following command.
		   g++ -march=native -O3 -funsafe-loop-optimizations -fno-math-errno -ffinite-math-only -fno-signed-zeros -fno-trapping-math -std=gnu++11 dictionary.h dictionary.cpp Main.cpp -lquadmath -o zipf

@par Usage: <program_name> <text_file>

@section todo_bugs_changelog Todo, Bugs, and Changelog

@todo Make program faster.

@bug None that I know of.

@par Changelog:
	@verbatim
	Date				Change
	-----------------	-------------------------------------------------------
	October  31, 2015	Created main.cpp and dictionary.h/.cpp, with associated
						dictionary class (just declarations, not definitions).
						Wrote potential hash function.
	November  1, 2015	Fixed error in dictionary::hash(). Added const global
						POW28 to dictionary.cpp to speed up hash calculation.
						Added hash variable in 'word' struct. Added more
						documentation. Added size and capacity count in
						dictionary class.
	November  5, 2015	Added getword(). Fixed integer division in hash calc.
						Added bool to dictionary class for whether there has
						been a hash collision or not. Added printUsage(),
						though it doesn't do anything yet.

						Changed hash to use __float128 if possible.

						Changed hash again to use typedef 'BFN' that can be
						easily set to whatever size you want.
	November  6, 2015	Modified hash to avoid potential problem if two words
						were identical, except one was one character longer and
						that character was an 'a'.
	November  7, 2015	Modified GCC compilation command.
	November  9, 2015	Changed dictionary list from 'word *' to 'word **'.
						Modified constructor and destructor accordingly. Added
						documentation for dictionary constructor and
						destructor. Changed dictionary insert() and remove() to
						take a number representing the number of copies of the
						specified word to insert/remove. Added documentation to
						dictionary insert(), find(), and remove(). Shifted hash
						result to the right by 1 'digit'. Added dictionary
						resize function and another private insert function.
						Wrote dictionary insert functions. Wrote dictionary
						find function. Wrote dictionary remove function. Added
						some preprocessor stuff to dictionary.h to simplify
						some things.
	November 10, 2015	Wrote printUsage().
	November 11, 2015	Fixed error in dictionary hash function.

						Added dictionary::size() to return the number of words
						in the dictionary. Wrote dictionary::print().

						Debugged getWord(), and dictionary hash() and print().
						Added some more documentation. Wrote logTime().
	November 12, 2015	Added code to print CSV file. Fixed bug in dictionary
						resize function. Fixed bug in dictionary hash function.
						Added some documentation. Fixed some formatting issues.

						Set pointer to nullptr after deleting in dictionary
						remove function.

						Changed one line to use reinterpret_cast for an
						approximately 20% performance gain.
	November 13, 2015	Removed safety checks from getword() for a 12.5%
						performance gain.

						Tried changing dictionary list to an array of words
						instead of an array of pointers to words. Program was
						significantly slower, so I didn't do that. Finally
						removed bool from dictionary class that was never used.
						Rearranged dictionary::word member variables for better
						data alignment, possibly. Made string member variable
						const. Added comments to dictionary::find() and
						dictionary::print().

						15% performance increase by initializing dictionary
						size based on input file size.
	November 14, 2015	Fixed dictionary::remove() and added some reinterpret
						casts. Changed reinterpret casts to a #define because
						it's shorter; also made them cast to const. This change
						seems to have actually slightly increased perfomance.
	November 15, 2015	Seperated dictionary resizing and insertion. Removed
						unnecessary usage of 'new' in dictionary::insert() for
						a performance increase of about 9%. Removed extra hash
						function that is now unused.
	November 17, 2015	Tried calculating hash with the probability a certain
						letter occurs first taken into account. Program was
						over 10 times slower, so I didn't do that. Changed
						filename extension removal to handle files without an
						extension and files with an extension not three letters
						long. Also moved it to after we know the file exists.
	November 26, 2015	Fixed some errors in dictionary.h's preprocessor code.
						Fixed dictionary::remove(), again. Added BLOOMISH
						define to dictionary files.

						Fixed mistype in dictionary.cpp's constant global array
						POW28 (1952 -> 21952). Changed one line in 
						dictionary::insert() to reduce the amount of data
						passed around. Slight performance increase.
	November 27, 2015	Fixed rare dictionary hash error. Slight performance
						decrease.
	November 28, 2015	Fully implemented dictionary BLOOMISH filter. There
						shouldn't be a performance cost when it's not being
						used, but there is a slight cost when it is being used.
						I have yet to test whether this cost is offset by the
						gain of using a smaller data type (I believe it is).
	November 29, 2015	Modified dictionary initialization size. Performance
						increased by 30% (when the given input file is the
						entire collected works of Shakespeare).
	November 30, 2015	Full testing shows a 60% performance increase from the
						code before Nov 26 to the current code, using BFN_DEF 3
						before, and BFN_DEF 1 with BLOOMISH now.
	@endverbatim
******************************************************************************/


#include <ctime>
#include <iostream>
#include <fstream>
#include <string>
#include "dictionary.h"


/**************************************************************************//**
@author John Colton

@par Description:
This function prints the usage instructions of this program to the console.
******************************************************************************/
void printUsage()
{
	std::cout << 
"Zipf's Law Program\n"
"-------------------------------------------------------------------------\n"
"This program calculates the number of occurences of every word in a given\n"
"text file. To run this program you must supply the name of a text file.";
}

/**************************************************************************//**
@author John Colton

@par Description:
This function returns the next word from an input stream. A word consists of
the letters A-z and possibly an apostrophe. The returned string will be in
lowercase and will not have an apostrophe at the start or end. This function
will not properly set the streams eofbit, or do any safety checks actually.

@param[in,out] stream - The input stream being read from.
@param[in,out] str - The word gotten.

@returns bool - True if a word was read.
******************************************************************************/
bool getword( std::istream & stream, std::string & str )
{
	// Clear the given string.
	str.clear();

	std::streambuf * sb = stream.rdbuf();

	char c; // temporary variable

	// While the next character in the stream is not a letter,
	while ( !isalpha( c = sb->sbumpc() ) )
	{
		// check that it's not the end of the stream.
		// If it is, return false.
		if ( c == EOF ) return false;
	}

	// c is a letter at this point, so we can start getting the word.
	str = tolower( c );

	while ( true )
	{
		// Get the next character from the stream.
		c = char( sb->sbumpc() );

		// If c is a letter or an apostrophe, add it to the word.
		if ( isalpha( c ) || c == '\'' ) str += tolower( c );

		else break;
	}

	// Remove trailing apostrophes, if there are any.
	if ( !str.empty() ) while ( str[str.length() - 1] == '\'' ) str.resize( str.length() - 1 );

	return true;
}

/**************************************************************************//**
@author John Colton

@par Description:
This function prints a message, along with the number of seconds elapsed since
the program started.

@param[in] message - The message to print.
******************************************************************************/
void logTime( const char * const message )
{
	std::cout.width( 20 );
	std::cout << std::left << message;
	std::cout.width( 10 ); std::cout.precision( 5 );
	std::cout << std::fixed << std::right << clock() / double( CLOCKS_PER_SEC )
			  << '\n';
}


/**************************************************************************//**
@author John Colton

@par Description:
This is the beginning of the program.

@param[in] argc - The number of arguments passed to this function.
@param[in] argv - The arguments passed to this function.

@returns int - The return code of this program.
******************************************************************************/
int main( int argc, char * argv[] )
{
	if ( argc != 2 )
	{
		printUsage();

		return 0;
	}


	std::cout << "Operation      Seconds Elapsed\n";
	logTime( "Initializing" );


	// Open the input file.
	std::ifstream fin( argv[1], std::ios_base::in | std::ios_base::binary );
	if ( !fin )
	{
		std::cout << argv[1] << " could not be opened.";

		return 0;
	}


	// Get the input filename as a std::string and remove its file extension.
	std::string filename( argv[1] );
	const size_t pLoc = filename.find_first_of( '.' );
	if ( pLoc != -1 ) filename.resize( pLoc );

	// Get Filesize
	const size_t filesize = fin.rdbuf()->pubseekoff( 0, std::ios_base::end, std::ios_base::in );
	fin.rdbuf()->pubseekoff( 0, std::ios_base::beg, std::ios_base::in );

	// Initialize dictionary size based on file size.
	dictionary dict( filesize / 120 );

	std::string word; word.reserve( 20 ); // temp variable

	size_t words = 0; // Number of words read from the input file.


	logTime( "Getting Words" );

	// Get every word from the file and insert it in the dictionary.
	while ( getword( fin, word ) )
	{
		dict.insert( word );

		++words;
	}


	logTime( "Printing Files" );

	// Open output files.
	std::ofstream wrd( filename + ".wrd" );
	std::ofstream csv( filename + ".csv" );
	
	// Print Headers
	wrd << "Zipf's Law: word concordance\n"
		   "----------------------------\n"
		   "File:         " << argv[1] << "\n"
		   "Total Words:  " << words << "\n"
		   "Unique Words: " << dict.size() << "\n\n";
	csv << "Zipf's Law,rank * freq = const\n\n"
		   "File," << argv[1] << "\n"
		   "Total Words," << words << "\n"
		   "Unique Words," << dict.size() << "\n\n";

	// Print Words and Frequencies
	dict.print( wrd, csv );


	logTime( "Program Complete" );
}
