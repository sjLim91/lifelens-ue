#pragma once

// Display-only text used by Observer mobile polish behavior consolidated from
// legacy PRs #30/#36/#38. Keep these strings out of simulation/Core code.
// The product-wide Korean/String Table migration can replace this centralized
// display layer without changing behavior contracts.
namespace LLObserverMobilePolishText
{
    inline const TCHAR* const BackHint = TEXT("‹ Back");
    inline const TCHAR* const NoResidents = TEXT("No residents yet");
    inline const TCHAR* const StripMorePrefix = TEXT("+");
    inline const TCHAR* const Ellipsis = TEXT("…");
}
