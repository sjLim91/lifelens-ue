#pragma once
#include <string>
#include <vector>
namespace lifelens {
struct MemoryEntry { int minute=0; std::string tag; double salience=0.0; };
struct MemoryState { std::vector<MemoryEntry> entries; };
}
