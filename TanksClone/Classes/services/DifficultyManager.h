#pragma once

#include "model/DifficultyBook.h"

namespace tanks
{
/** Loads the difficulty modes and answers which one the profile has chosen. */
class DifficultyManager
{
public:
	void Init();

	const model::DifficultyDef& GetCurrent() const;
	void SelectNext();
	const model::DifficultyBook& GetBook() const;

private:
	model::DifficultyBook Book;
};
} // namespace tanks
