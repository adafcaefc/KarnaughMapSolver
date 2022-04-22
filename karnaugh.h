#pragma once

#include <map>
#include <unordered_map>
#include <filesystem>

namespace karnaugh
{
    // ivec2 is a standard 2-integer vector class
    struct ivec2
    {
        int x = 0, y = 0;
        ivec2() { x = y = 0; }
        ivec2(int _x, int _y) : x(_x), y(_y) {};
    };

    // kgroup is a standard rect class 
    // consisting off starting point and size 
    struct kgroup
    {
        ivec2 start, size;
        kgroup(const ivec2& _start, const ivec2& _size) : size(_size), start(_start) {};

        bool is_in(const kgroup& group) const;
        std::vector<ivec2> to_points() const;
    };

    struct kmap
    {
    public:

        // declarations so it's easier to write variable type
        using var_map_t = std::map<char, bool>;
        using var_pmap_t = std::pair<var_map_t, int>;
        using var_list_t = std::vector<char>;
        using var_coord_t = std::pair<var_pmap_t, var_pmap_t>;

        // this contains the variable information
        // the unordered variables contains all of the variables,
        // which is stored in char, in a vector
        // the variables member stores it in pair
        // one for the horizontal axis and the other one
        // for the vertical axis
        var_list_t unordered_variables;
        std::pair<var_list_t, var_list_t> variables;

        // this constructor takes in a file path as its argument
        // A B C   <--- first line will always contain the variable name
        // 0 0 0 0 <--- this means the value for A = 0, B = 0, C = 0 is 0
        // 0 0 1 1 <--- this means the value for A = 0, B = 0, C = 1 is 1
        // ...
        // the file can have variying amount of variables
        kmap(const std::filesystem::path& path);

        // these are the common size functions
        size_t size_x() const { return 1 << this->variables.first.size();  }
        size_t size_y() const { return 1 << this->variables.second.size(); }
        size_t size()   const { return this->size_x() * this->size_y();    }
        bool   empty()  const { return this->size() == 0u;                 }

        // these are the functions to access the indexed data from
        // the internal map, the x and y axis are counter from
        // top left going to bottom right
        const bool*        value_for     (const int x, const int y) const;
        const var_coord_t* var_coord_for (const int x, const int y) const;
        const var_map_t*   var_map_for_x (const int x) const;
        const var_map_t*   var_map_for_y (const int y) const;

        // these are the functions to convert the map into formula
        // the enum is for the variable formula
        // and whether a variable is affecting the value 
        enum class k_var_result { k_vtrue, k_vfalse, k_vnull };
        std::vector<std::map<char, k_var_result>> get_formula(const bool v) const;
        std::string get_formula_string(const bool v) const;

    private:

        // this is the internal map to store the actual Karnaugh Map
        // the type of this variable is as follow
        // it's a vector of a pair of coordinate and its value in bool
        // the coordinate consists of two pmaps, one for the horizontal axis
        // and the other one for vertical axis
        // the pmap itself contains the variable index in integer
        // and a std::map<char, bool> for the variable values present
        // in the corresponding coordinate
        std::vector<std::pair<var_coord_t, bool>> internal_map;

        // these are the functions used in the constructor
        // to load the input string stream into the map
        void load_variables (std::istringstream& iss);
        void load_values    (std::istringstream& iss);
        
        // these are the functions related to getting the groups
        // to do : fix for rollover groups
        std::vector<kgroup> get_all_groups      (const bool v) const;
        std::vector<kgroup> get_filtered_groups (const bool v) const;

        // these are internal functions related to groups
        bool check_group(const kgroup& group, const bool v) const;
        std::vector<var_coord_t const*> coords_from_kgroup(const kgroup& group) const;
    };
}