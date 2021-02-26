#include "id.hpp"

id_type get_next_id() { static size_t last = 0; return {last++}; }
