//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Placeholder

#ifndef COGS_HEADER_PARSER_REGEXP
#define COGS_HEADER_PARSER_REGEXP


#include "cogs/math/range_to_int.hpp"


#error ?

namespace cogs {


// expression_builder uses a size_t token, and a given max_token.  Token values
// larger than max_token should not be used.  A table of max_token elements is
// used.  This is reasonable for 'char' parsers, but using larger values may result in very
// inefficient use of memory.  Instead, reduce the range of tokens (with another layer/class).

template <size_t token_count = 256>
class expression
{
public:
	typedef typename range_to_int_t<0, token_count - 1> token_t;

	typedef expression<token_count> this_t;

private:
	token_t m_tokenClassTable[token_count]; // token -> tokenClass

	array<token_t> m_tokenClassCounts; // tokenClass -> token

	class state
	{
	public:
		array<size_t> m_table;
		bool m_possibleEndState;

		state()
			: m_possibleEndState(0)
		{ }
	};

	array<state> m_stateTables;

	size_t get_num_token_classes() const { return m_tokenClassCounts.length(); }
	size_t get_num_states() const { return m_stateTables.length(); }

	// illegal to pass duplicate tokens, n==0, or n>=token_count
	static this_t literal_or_exclusion_set(token_t* t, size_t n, bool exclude)
	{
		this_t e;
		for (size_t i = 0; i < n; i++)
			e.m_tokenClassTable[t[i]] = 1;
		e.m_tokenClassCounts[0] = token_count - n;
		e.m_tokenClassCounts.append(n);
		e.m_stateTables.grow(1);
		e.m_stateTables[0].m_table.grow(2);
		e.m_stateTables[0].m_table[exclude ? 1 : 0] = const_max_int_v<size_t>; // default char class (0) is fail
		e.m_stateTables[0].m_table[exclude ? 0 : 1] = 1; // literal char class (1) is success
		return e;
	}

	void add_literal_or_exclusion(token_t t, bool exclude)
	{
		token_t tokenClass = m_tokenClassTable[t];
		if (m_tokenClassCounts[tokenClass] > 1) // needs its own token class
		{
			--(m_tokenClassCounts[tokenClass]);
			size_t originalTokenClass = tokenClass; // Will need this to stretch tables
			tokenClass = get_num_token_classes();
			m_tokenClassTable[t] = tokenClass;
			m_tokenClassCounts.append(1);

			// Grow all state tables by 1.
			size_t numStates = get_num_states();
			for (size_t i = 0; i < numStates; i++)
				m_stateTables[i].m_table.append(m_stateTables[i].m_table[originalTokenClass]);
		}

		// Add a state for the literal
		size_t numTokenClasses = get_num_token_classes();
		size_t curState = get_num_states();
		m_stateTables.grow(1);
		m_stateTables[curState].m_table.resize(numTokenClasses);

		array<size_t>& curTable = m_tables[curState].m_table;

		size_t matched_transition = get_num_states();
		size_t unmatched_transition = const_max_int_v<size_t>;
		if (exclude)
		{
			unmatched_transition  = matched_transition;
			matched_transition = const_max_int_v<size_t>;
		}

		// Default all transitions to fail state (-1)
		for (size_t i = 0; i < tokenClass; i++)
			curTable[i] = unmatched_transition;
		curTable[tokenClass] = matched_transition;
		for (size_t i = tokenClass + 1; i < numTokenClasses; i++)
			curTable[i] = unmatched_transition;
	}

	static this_t literal_or_exclusion_string(token_t* t, size_t n, bool exclude)
	{
		for (size_t i = 0; i < n; i++)
			add_literal_or_exclusion(t[i], exclude);
	}


public:
	expression()
	{
		m_tokenClassCounts.append((token_t)token_count);

		for (size_t i = 0; i < token_count; i++)
			m_tokenClassTable[i] = 0;
	}

	static this_t literal(token_t t) { return literal_or_exclusion_set(&t, 1, false); }
	static this_t literal(token_t* t, size_t n) { return literal_or_exclusion_string(t, n, false); }
	static this_t literal_set(token_t* t, size_t n) { return literal_or_exclusion_set(t, n, false); }

	static this_t exclusion(token_t t) { return literal_or_exclusion_set(&t, 1, true); }
	static this_t exclusion(token_t* t, size_t n) { return literal_or_exclusion_string(t, n, true); }
	static this_t exclusion_set(token_t* t, size_t n) { return literal_or_exclusion_set(t, n, true); }

	// OR is the next simplest operation.
	// A character class is created for each unique combination of character classes (e1<->e2)
	// A state is created for each unique combination of state transitions (e1<->e2)
	static this_t or(const this_t& e1, const this_t& e2)
	{
		this_t e;
		// First merge the two character class tables
		// We do this by creating a map from each pair of e1/e2 classes, to new classes.

		// We will traverse the new char class table, when combining state tables
		array<pair<token_t, token_t> > newCharClassMap; // maps back to char class
	}

	// concat
	// A character class is created for each unique combination of character classes (e1<->e2)
	// Acts as an OR on all possible end states.
	// On definite end state, appends.
	static this_t concat(const this_t& e1, const this_t& e2);
};


}


#endif
