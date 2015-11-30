/**************************************************************************//**
@file

@brief This file defines the dictionary class.

The hash value of a word is stored as a floating point value. Due to floating
point imprecision, there is a limit to the number of characters a word can
have depending on the size of the floating point type used. However, smaller
floating point types will be faster than larger ones. If the chosen data type
is too small, similar words may be evaluated as if they were the same word.
  Size   Max Characters   Note
 16bit    2               Not sure what you can do with this.
 32bit    4               Still not much room.
 64bit   10               Enough for all words <= 10 characters long.
 80bit   13               Enough for all words <= 13 characters long.
128bit   23               Enough for all English words.

Although some compilers support 80bit floating point numbers, there is no 80bit
integer, so it may be considerably slower to use.
******************************************************************************/

#ifndef DICTIONARY_H
#define DICTIONARY_H

#include <string>

// The data type to use.
// 0 = float (32 bit)
// 1 = double (64 bit)
// 2 = long double (80 bit, unless you're using MSVC++, then it's 64 bit)
// 3 = __float128 (128 bit, only available in GCC >= v4.6)
#define BFN_DEF 1

// Set to 'true' to use something like a Bloom Filter. This will allow you to
// use a smaller data type than necessary to get correct results. Set this to
// 'false' if your chosen data type is large enough on its own. If your chosen
// data type is not supported by your compiler, a smaller type will be chosen
// and 'BLOOMISH' will be set to 'true'.
#define BLOOMISH true


// BFN = Big Floating-Point Number
// BIN = Big Integer Number
// Used for Hashing
// Don't touch this chunk of preprocessor stuff.
#if BFN_DEF == 3
	#if !( __GNUC__ > 4 || ( __GNUC__ == 4 && __GNUC_MINOR__ >= 6 ) )
		#undef BFN_DEF
		#define BFN_DEF 2
		#undef BLOOMISH
		#define BLOOMISH true
	#endif
#endif
#if BFN_DEF == 0
	typedef float BFN;
	typedef uint32_t BIN;
	#define DICT_MAX_CHARS 5
#elif BFN_DEF == 1
	typedef double BFN;
	typedef uint64_t BIN;
	#define DICT_MAX_CHARS 11
#elif BFN_DEF == 2
	typedef long double BFN;
	#ifdef _MSC_VER
		typedef uint64_t BIN;
		#define DICT_MAX_CHARS 11
		#undef BLOOMISH
		#define BLOOMISH true
	#else
		typedef long double BIN; // There is no 80-bit integer.
		#define DICT_MAX_CHARS 14
	#endif
#elif BFN_DEF == 3 // GCC >= 4.6 Only
	typedef __float128 BFN;
	typedef unsigned __int128 BIN;
	#define DICT_MAX_CHARS 24
#endif

#undef BFN_DEF


/*! @class
	@brief The 'dictionary' class declaration. */
class dictionary
{
	private:

	/*! @struct
		@brief The 'word' struct definition. */
	struct word
	{
		word( const std::string & W, const size_t N, const BFN & H ) : hash( H ), num( N ), str( W ) {}

		BFN hash; //!< The hash of this word.

		size_t num; //!< The number of times this word appears.

		const std::string str; //!< The word to store.
	};


	public:
	
	/**********************************************************************//**
	@author John Colton

	@par Description:
	This function initializes the dictionary.

	@param[in] num - The number of words to allocate space for.
	**************************************************************************/
	dictionary( const size_t num = 1000 );
	/**********************************************************************//**
	@author John Colton

	@par Description:
	This function deletes the dictionary.
	**************************************************************************/
	~dictionary();
	
	/**********************************************************************//**
	@author John Colton

	@par Description:
	This function adds a word to the dictionary.

	@param[in] str - The word to add.
	@param[in] num - The number of copies of the word to add. Defaults to 1.

	@returns size_t - The number of copies of the word now in the dictionary.
	**************************************************************************/
	size_t insert( const std::string & str, const size_t num = 1 );
	/**********************************************************************//**
	@author John Colton

	@par Description:
	This function finds a word to the dictionary.

	@param[in] str - The word to find.

	@returns size_t - The number of copies of the word in the dictionary.
	**************************************************************************/
	size_t find( const std::string & str ) const;
	/**********************************************************************//**
	@author John Colton

	@par Description:
	This function removes a word from the dictionary.

	@param[in] str - The word to remove.
	@param[in] num - The number of copies of the word to remove.
	                 Defaults to -1 (all).

	@returns size_t - The number of copies of the word now in the dictionary.
	**************************************************************************/
	size_t remove( const std::string & str, const size_t num = -1 );

	/**********************************************************************//**
	@author John Colton

	@par Description:
	This function returns the number of words in the dictionary.

	@returns size_t - The number words in the dictionary.
	**************************************************************************/
	size_t size();
	/**********************************************************************//**
	@author John Colton

	@par Description:
	This function prints the contents of this dictionary to an output stream.

	@param[in,out] out - The output stream to print to.
	**************************************************************************/
	void print( std::ostream & txt, std::ostream & csv );


	private:
	
	/**********************************************************************//**
	@author John Colton

	@par Description:
	This function adds a word to the dictionary. Since this function is only
	called by the other insert() and by resize(), it is assumed that the word
	does not already exist in the list.

	@param[in] W - The word to add.

	@returns size_t - The number of copies of the word now in the dictionary.
	**************************************************************************/
	size_t insert( word * W );

	/**********************************************************************//**
	@author John Colton

	@par Description:
	This function resizes the dictionary.

	@param[in] newSize - The new capacity of the dictionary.
	**************************************************************************/
	void resize( const size_t newSize );

	/**********************************************************************//**
	@author John Colton

	@par Description:
	This function calculates the hash of a string. It assumes the string only
	contains apostrophes (not at the start or end) and lowercase letters.

	The idea is to read the string as a number in base 27 where an apostrophe
	is 0 and lowercase letters are numbered from 1 to 26 alphabetically. Each
	letter of the string is treated as a 'digit' after the decimal, resulting
	in a number between 0 and 1. For example, "foo" would be converted to the
	base 27 number "0.5FF", or (5/27)+(15/28^2)+(15/28^3), or
	0.20500114728431054961667206565166 in base 10. (The first character cannot
	be an apostrophe, so its value is reduced by 1 so that hashes will start at
	0 instead of 1/27th.)

	@param[in] str - The string to hashify.

	@returns BFN - The hash value of the given string.
	**************************************************************************/
	BFN hash( const std::string & str ) const;


	struct { size_t capacity, size; } count = { 0, 0 };

	word ** list = nullptr; //!< A pointer to the hash table.
};


#endif /* DICTIONARY_H */
