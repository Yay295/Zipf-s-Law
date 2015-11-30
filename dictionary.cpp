#include <iostream>
#include <vector>
#include "dictionary.h"


// Just for convenience.
#define RE_CAST_BIN reinterpret_cast<const BIN&>

// For comparing words.
#define LOC_H_LT_HINT RE_CAST_BIN((*location)->hash) < Hint
#define LOC_H_IS_HINT RE_CAST_BIN((*location)->hash) == Hint
#if BLOOMISH
	#define LOC_STR_LT_STR (*location)->str < str
	#define LOC_STR_IS_STR (*location)->str == str
#else
	#define LOC_STR_LT_STR true
	#define LOC_STR_IS_STR true
#endif

// Overloaded std::string compare so it works the way I want.
inline bool operator < ( const std::string & lhs, const std::string & rhs)
{
	// Get the length of the shorter of the two strings.
	const size_t length = ( lhs.length() < rhs.length() ? lhs.length() : rhs.length() );

	// Compare the strings up to that length.
	for ( size_t i = 0; i < length; ++i )
	{
		if ( lhs[i] < rhs[i] ) return true;

		else if ( lhs[i] > rhs[i] ) return false;
	}

	// If they are equal up to that point, check their lengths.
	if ( lhs.length() >= rhs.length() ) return false;

	return true;
}


/*!
@brief A list of numbers 28^n, for 0 < n < 24.
*/
const BFN POW28[25] = {				 28.L,
									784.L,
								  21952.L, // < 2^16
								 614656.L,
							   17210368.L,
							  481890304.L, // < 2^32
							13492928512.L,
						   377801998336.L,
						 10578455953408.L,
						296196766695424.L,
					   8293509467471872.L,
					 232218265089212416.L,
					6502111422497947648.L, // < 2^64
				  182059119829942534144.L,
				 5097655355238390956032.L,
			   142734349946674946768896.L,
			  3996561798506898509529088.L,
			111903730358193158266814464.L,
		   3133304450029408431470804992.L,
		  87732524600823436081182539776.L,
		2456510688823056210273111113728.L,
	   68782299287045573887647111184384.L,
	 1925904380037276068854119113162752.L,
	53925322641043729927915335168557056.L,
  1509909033949224437981629384719597568.L };
/* Windows 10 calculator ran out of digits for the last three...
340282366920938463463374607431768211456 == 2^128 */


dictionary::dictionary( const size_t num )
{
	// Allocate space for 'num' word pointers and set them to nullptr. One
	// extra spot is allocated to assist in overflow detection.
	list = new word*[num+1]();

	count.capacity = num;
}

dictionary::~dictionary()
{
	// Delete the words in the list.
	for ( size_t i = 0; i < count.capacity; ++i )
		delete list[i];

	// Delete the array of word pointers.
	delete[] list;
}


size_t dictionary::insert( const std::string & str, const size_t num )
{
	if ( !str.empty() )
	{
		// If the list is over 75% full, resize it.
		if ( 4 * count.size > 3 * count.capacity )
		{
			resize( count.capacity << 1 );

			return insert( str, num );
		}

		// Get the hash of the word to insert.
		const BFN H = hash( str );
		// Convert it to an int.
		const BIN Hint = RE_CAST_BIN(H);

		// The location that the hash of the word to insert points to.
		word ** location = &list[size_t(H*count.capacity)];

		// While the location is not a nullptr, and the hash at the location is
		// less than the hash of string being inserted.
		while ( *location && LOC_H_LT_HINT )
			++location; // Increment the location.

		#if BLOOMISH
		// While the location is not a nullptr, and the string at the location
		// is alphabetically 'less than' the string being inserted.
		while ( *location && LOC_STR_LT_STR )
			++location; // Increment the location.
		#endif

		// If we have found an empty spot,
		if ( *location == nullptr )
		{
			// check that it's within the space allocated for words. If it's
			// not, resize the list.
			if ( location == &list[count.capacity] )
			{
				resize( count.capacity << 1 );

				// We know the word isn't in the list now, so we can allocate
				// new space for it. We then need to increment 'count.size'.
				++count.size; return insert( new word( str, num, H ) );
			}

			*location = new word( str, num, H ); // Otherwise, insert the word in that spot,

			++count.size; // increment the count of words in the list, and

			return num; // return 'num'.
		}

		// If the word at this location is the same as the word we are
		// inserting, increment the counter for that word and return it.
		if ( LOC_H_IS_HINT && LOC_STR_IS_STR )
			return ( (*location)->num += num );

		// At this point the word being inserted occurs alphabetically before
		// the word at 'location', so we can create a new 'word' struct because
		// we know the word being inserted doesn't yet exist in the list.
		word * temp = new word( str, num, H ); ++count.size;

		// Then, while 'location' points to a word,
		while ( *location )
		{
			std::swap( *location, temp ); // swap that word ptr with temp and

			++location; // increment 'location'.
		}

		// If we are now past the end of the list,
		if ( location == &list[count.capacity] )
		{
			resize( count.capacity << 1 ); // resize the list and

			insert( temp ); // insert the last word we swapped.
		}

		// If we are not past the end of the list,
		else *location = temp; // just insert that word in this spot.

		return num;
	}

	return 0;
}

size_t dictionary::find( const std::string & str ) const
{
	// If the string is not blank.
	if ( !str.empty() )
	{
		// Get the string's hash.
		const BFN H = hash( str );
		// Convert it to an int.
		const BIN Hint = RE_CAST_BIN( H );

		// Get a pointer to the location that the string should be in the list.
		word ** location = &list[size_t(H*count.capacity)];

		// If that location is occupied by a string that is not this string,
		// increment 'location'.
		while ( *location && LOC_H_LT_HINT )
			++location;
		#if BLOOMISH
		while ( *location && LOC_STR_LT_STR )
			++location;
		#endif

		// If we have now found the string, return the number of occurences of
		// it we have stored.
		if ( *location && LOC_H_IS_HINT && LOC_STR_IS_STR )
			return (*location)->num;
	}

	// Otherwise return 0.
	return 0;
}

size_t dictionary::remove( const std::string & str, const size_t num )
{
	if ( !str.empty() )
	{
		// Get the hash of the string.
		const BFN H = hash( str );
		// Convert it to an int.
		const BIN Hint = RE_CAST_BIN(H);

		// Get a pointer to the location the string should be in the list.
		word ** location = &list[size_t(H*count.capacity)];

		// While the location is not a nullptr, and the hash at the location is
		// less than the hash of string being removed.
		while ( *location && LOC_H_LT_HINT )
			++location; // Increment the location.

		#if BLOOMISH
		// While the location is not a nullptr, and the string at the location
		// is alphabetically 'less than' the string being removed.
		while ( *location && LOC_STR_LT_STR )
			++location; // Increment the location.
		#endif

		// If 'location' now points to the word to remove.
		if ( *location && LOC_H_IS_HINT && LOC_STR_IS_STR )
		{
			// If we are removing more (or as many) occurences of that word
			// than we have stored, delete the word.
			if ( num >= (*location)->num )
			{
				delete *location;

				// While there is another word right after this one and it is
				// not in the location it should be, move it up one space.
				while ( *(location+1) && location >= &list[size_t(((*(location+1))->hash)*count.capacity)] )
				{
					*location = *(location+1);

					++location;
				}

				*(location+1) = nullptr;

				--count.size;
			}

			// Otherwise decrement that words' counter and return its value.
			else return ( (*location)->num -= num );
		}
	}

	return 0;
}


size_t dictionary::size()
{
	return count.size;
}

void dictionary::print( std::ostream & txt, std::ostream & csv )
{
	// This vector contains all of the words in the dictionary,
	// sorted by frequency.
	std::vector<std::vector<std::string>> frequency;

	size_t printed = 0; // Number of words that have been printed.


	// for ( words in the dictionary )
	for ( size_t i = 0; i < count.capacity; ++i )
	{
		// If there is a word at this location,
		if ( list[i] )
		{
			// and it's frequency is greater than the frequency vector is
			// capable of holding, increase the size of the frequency vector
			// so that it can be inserted.
			if ( list[i]->num > frequency.size() ) frequency.resize( list[i]->num );

			// Insert the word into the frequency vector appropriately.
			frequency[list[i]->num-1].push_back( list[i]->str );
		}
	}


	// Headers
	txt << "Word Frequencies                             Ranks     Avg Rank\n"
		   "----------------                             -----     --------";
	csv << "Rank,Frequency,Rank x Frequency";


	// for ( frequencies in the frequency vector )
	for ( int i = frequency.size() - 1; i >= 0; --i )
	{
		// If there are words with this frequency.
		if ( !frequency[i].empty() )
		{
			// Print the header for this frequency level.
			txt << "\n\nWords occuring " << i + 1 << " time" << ( i ? "s:" : ":" );

			// Convert the range of ranks covered by this frequency to a
			// string, and calculate the average rank.
			std::string ranks;
			double avg = double( printed + 1 );
			if ( frequency[i].size() > 1 )
			{
				ranks = std::to_string( printed + 1 ) + '-' + std::to_string( printed + frequency[i].size() );
				printed += frequency[i].size();
			}
			else ranks = std::to_string( ++printed );
			avg = avg + ( printed - avg ) / 2.0;

			// Print the range of ranks and the average rank.
			txt.width( 28 - std::to_string( i + 1 ).size() + ( i ? 0 : 1 ) );
			txt << std::right << ranks;
			txt.width( 13 ); txt.precision( 1 );
			txt << std::fixed << std::right << avg;

			// Print the words that occur at this frequency level.
			for ( size_t j = 0; j < frequency[i].size(); ++j )
			{
				if ( !( j % 5 ) ) txt << '\n';
				txt.width( 15 );
				txt << std::left << frequency[i][j];
			}

			// Output CSV Info
			csv << '\n' << avg << ',' << i + 1 << ',' << avg * ( i + 1 );
		}
	}
}


size_t dictionary::insert( word * W )
{
	// If the list is over 75% full, resize it.
	if ( 4 * count.size > 3 * count.capacity )
	{
		resize( count.capacity << 1 );

		return insert( W );
	}

	// The location that the hash of the word to insert points to.
	word ** location = &list[size_t(W->hash*count.capacity)];

	// W's hash, as an integer.
	const BIN Hint = RE_CAST_BIN(W->hash);
	// A reference to W's string for the #define'd comparisons.
	const std::string & str = W->str;

	// While the location is not a nullptr, and the hash at the location is
	// less than the hash of the string being inserted.
	while ( *location && LOC_H_LT_HINT )
		++location; // Increment the location.

	#if BLOOMISH
	// While the location is not a nullptr, and the string at the location
	// is alphabetically 'less than' the string being inserted.
	while ( *location && LOC_STR_LT_STR )
		++location; // Increment the location.
	#endif

	// If we have found an empty spot,
	if ( *location == nullptr )
	{
		// check that it's within the space allocated for words. If it's
		// not, resize the list.
		if ( location == &list[count.capacity] )
		{
			resize( count.capacity << 1 );

			return insert( W );
		}

		*location = W; // Otherwise, insert the word in that spot and

		return W->num; // return 'num'.
	}

	// At this point the word being inserted occurs alphabetically before
	// the word at 'location', so we will make a copy of 'W->num',
	const size_t temp = W->num;
	// and then while 'location' points to a word,
	while ( *location )
	{
		std::swap( *location, W ); // swap that word ptr with W and

		++location; // increment 'location'.
	}

	// If we are now past the end of the list,
	if ( location == &list[count.capacity] )
	{
		resize( count.capacity << 1 ); // resize the list and

		insert( W ); // insert the last word we swapped.
	}

	else *location = W; // Otherwise, insert that word in this spot.

	return temp;
}

void dictionary::resize( const size_t newSize )
{
	std::cout << "    Resizing List\n";


	word ** oldList = list;

	const size_t oldSize = count.capacity;


	// Allocate space for 'num' word pointers and set them to nullptr. One
	// extra spot is allocated to assist in overflow detection.
	list = new word*[newSize+1]();

	count.capacity = newSize;

	// Move all of the words from the old list to the new list.
	for ( size_t i = 0; i < oldSize; ++i )
		if ( oldList[i] ) insert( oldList[i] );


	delete[] oldList;
}

BFN dictionary::hash( const std::string & str ) const
{
	if ( !str.empty() )
	{
		// The first character won't be an apostrophe, so there's no need to
		// check for that.
		BFN hash = BFN( str[0] - 'a' ) / BFN( 27 );

		// The lesser value between the length of the string and the maximum
		// number of characters that can be represented in the chosen hash
		// variable type.
		const size_t length = ( str.size() > DICT_MAX_CHARS ? DICT_MAX_CHARS : str.size() );

		for ( size_t i = 1; i < length; ++i )
		{
			// If the character is a lowercase letter.
			if ( str[i] != '\'' ) hash += BFN( str[i] - '`' ) / POW28[i];
		}

		// Get one extra character if needed. This is important in the case
		// where DICT_MAX_CHARS is less than the size of a string. If that
		// happens, then it is possible to have two strings: ??? and ???' (if
		// DICT_MAX_CHARS == 4). In this case, both strings would have the same
		// hash. To give the second string a different hash, we basically
		// extend 'length' by 1. The chosen BFN is too small to completely
		// support one extra character, but it's enough to differentiate the
		// two examples given above.
		if ( str.size() > length && str[length-1] == '\'' )
			hash += BFN( str[length] - '`' ) / POW28[length];

		return hash;
	}

	return BFN( -1 );
}
