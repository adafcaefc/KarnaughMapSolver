#include "karnaugh.h"

#include <map>
#include <unordered_map>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <numeric>

namespace karnaugh
{
    namespace utils
    {
        // There are 9 types of available pattern
        // for 4 variables karnaugh maps
        // 
        // 1.     2.         3.  
        // x      x x        x   
        //                   x
        //        4.
        //        x x x x
        // 
        // 5.     6.         7.
        // x x    x x x x    x x
        // x x    x x x x    x x
        //                   x x
        // 8.           9.   x x
        // x x x x      x
        // x x x x      x
        // x x x x      x
        // x x x x      x
        // 
        // we will denote them by the x and y sizes
        static const std::vector<ivec2> patterns =
        {
            { 1, 1 }, { 2, 1 }, { 1, 2 }, { 4, 1 }, { 1, 4 }, { 2, 2 }, { 4, 2 }, { 2, 4 }, { 4, 4 }
        };

        // generates an empty map from the key list
        template<typename T_map, typename T_iter>
        T_map map_from_keys(const T_iter& keys)
        {
            T_map map;
            for (auto& v : keys) { map[v]; }
            return map;
        }

        // generate karnaugh order (size = 2 : 00, 01, 11, 10)
        // this is hardcoded due to the fact that size 3+ requires a different set of order for A and B 
        // 6 variables K-map is not implemented yet as of now
        std::vector<std::vector<int>> generate_order(const size_t size)
        {
            switch (size)
            {
            case 1:
                return { { 0 }, { 1 } };
            case 2:
                return { { 0, 0 }, { 0, 1 }, { 1, 1 }, { 1, 0 } };
            default:
                return { };
            }
        }
    }

    // check whether a kgroup is inside another group
    bool kgroup::is_in(const kgroup& group) const
    {
        return
            this->start.x >= group.start.x &&
            this->start.y >= group.start.y &&
            (this->start.x + this->size.x <= group.start.x + group.size.x) &&
            (this->start.y + this->size.y <= group.start.y + group.size.y);
    }
    
    // check whether a kgroup is inside another group
    std::vector<ivec2> kgroup::to_points() const
    {
        std::vector<ivec2> result;
        for (int x = 0; x < this->size.x; ++x)
        {
            for (int y = 0; y < this->size.y; ++y)
            {
                result.push_back(ivec2(this->start.x + x, this->start.y + y));
            }
        }
        return result;
    }

    // loads the value of a certain x, y point on the map
    // returns nullptr if it cannot find the correct item
    const bool* kmap::value_for(const int x, const int y) const
    {
        for (auto& [k, v] : this->internal_map)
        {
            const auto& [cx, cy] = k;
            if (cx.second == x && cy.second == y) { return &v; }
        }
        return nullptr;
    }

    // loads the variable state of a certain x, y point on the map
    // returns nullptr if it cannot find the correct item
    const var_coord_t* kmap::var_coord_for(const int x, const int y) const
    {
        for (auto& [k, v] : this->internal_map)
        {
            const auto& [cx, cy] = k;
            if (cx.second == x && cy.second == y) { return &k; }
        }
        return nullptr;
    }

    // loads the variable state of a certain x point on the map
    // returns nullptr if it cannot find the correct item
    const var_map_t* kmap::var_map_for_x(const int x) const
    {
        for (auto& [k, v] : this->internal_map)
        {
            const auto& [cx, cy] = k;
            if (cx.second == x) { return &cx.first; }
        }
        return nullptr;
    }

    // loads the variable state of a certain y point on the map
    // returns nullptr if it cannot find the correct item
    const var_map_t* kmap::var_map_for_y(const int y) const
    {
        for (auto& [k, v] : this->internal_map)
        {
            const auto& [cx, cy] = k;
            if (cy.second == y) { return &cy.first; }
        }
        return nullptr;
    }

    // loads the variables name
    // A B C   <--- first line will always contain the variable name
    // 0 0 0 0 
    // 0 0 1 1
    // ...
    // the points are loaded as a pair (first, second)
    // it will also place all the variables in the variables vector
    void kmap::load_variables(std::istringstream& iss)
    {
        std::vector<char> tvariables;
        std::pair<var_map_t, var_map_t> tmap;
        char temp = 0;
        while (iss >> temp) { tvariables.push_back(temp); }

        // we will put them in a temporary map to deduce the order
        const auto s = tvariables.size();
        for (auto i = 0u; i < s; ++i)
        {
            // this splits the variables into two, splitting them in x and y axis
            // the first half will go into the first item in the pair and 
            // the second will go into the second one
            const auto n = s % 2 == 0 ? i < s / 2 : i <= s / 2;
            if (n) { tmap.first.emplace(tvariables.at(i), false); }
            else { tmap.second.emplace(tvariables.at(i), false); }
        }

        for (auto& [k, v] : tmap.first)
        {
            this->variables.first.push_back(k);
        }

        for (auto& [k, v] : tmap.second)
        {
            this->variables.second.push_back(k);
        }

        this->unordered_variables = tvariables;
    }

    // loads the value
    // A B C   
    // 0 0 0 0 <--- this means the value for A = 0, B = 0, C = 0 is 0
    // 0 0 1 1 <--- this means the value for A = 0, B = 0, C = 1 is 1
    // ...
    // the points are loaded as a copy because it will be modified in the future
    void kmap::load_values(std::istringstream& iss)
    {
        int temp = 0;
        std::vector<int> result;
        while (iss >> temp) { result.push_back(temp); }

        const auto process = [this](const var_list_t& list, const std::vector<int>& vars) -> var_pmap_t
        {
            auto temp = utils::map_from_keys<var_map_t>(list);

            for (auto i = 0u; i < vars.size(); ++i)
            {
                try { temp.at(this->unordered_variables.at(i)) = vars.at(i); }
                catch (const std::exception& e) {}
            }

            const auto order = utils::generate_order(temp.size());

            std::vector<int> current_vmap;
            for (auto& [k, v] : temp) { current_vmap.push_back(v); }

            int position = 0;
            for (auto i = 0u; i < order.size(); ++i)
            {
                if (order.at(i) == current_vmap)
                {
                    position = i;
                    break;
                }
            }

            return { temp, position };
        };

        const auto f = process(this->variables.first, result);
        const auto s = process(this->variables.second, result);

        this->internal_map.emplace_back(std::pair(std::pair(f, s), result.back()));
    }

    // load the map from a path
    kmap::kmap(const std::filesystem::path& path)
    {
        if (!std::filesystem::exists(path)) { return; }

        bool load_variable = false;

        std::string line;
        std::ifstream in(path);
        while (std::getline(in, line))
        {
            std::istringstream iss(line);
            if (!load_variable)
            {
                // the first line contains the variable name
                // this will only be run once
                this->load_variables(iss);
                load_variable = true;
                continue;
            }
            this->load_values(iss);
        }
    }

    // get all groups with the same value from the map
    // this can be done by checking for every single submatrix
    // that is present using the patterns defined above
    // if all value of the matrix is the same it will be included
    // into the result
    std::vector<kgroup> kmap::get_all_groups(const bool v) const
    {
        std::vector<kgroup> result;
        for (auto& mask : utils::patterns)
        {
            for (int x = 0; x < static_cast<int>(this->size_x()) - mask.x + 1; ++x)
            {
                for (int y = 0; y < static_cast<int>(this->size_y()) - mask.y + 1; ++y)
                {
                    kgroup current(ivec2(x, y), mask);
                    if (this->check_group(current, v)) { result.push_back(current); }
                }
            }
        }
        return result;
    }

    // this will filter duplicate groups, done in two pass
    std::vector<kgroup> kmap::get_filtered_groups(const bool v) const
    {
        std::vector<kgroup> result;
        const auto groups = this->get_all_groups(v);

        // first pass removes all group that is fully inside other groups
        // for example
        // 
        // x x x x
        // x o o x
        // x o o x 
        // x x x x
        //
        // group o is fully inside group x
        for (auto& g : groups)
        {
            bool is_in = false;
            for (auto& b : groups)
            {
                if (g.is_in(b) && &g != &b) { is_in = true; break; }
            }
            if (!is_in) { result.push_back(g); }
        }

        // second pass removes group whose members are found in other groups
        // 
        // .[x x]x
        // . . . x
        // 
        // . x[x x]
        // . . . x
        //        
        // . x x[x]
        // . . .[x]
        // 
        // in this example, the second group will be removed but 
        // the first and third group wont because they have a unique item
        const auto tresult = result;
        result.clear();

        for (auto& g : tresult)
        {
            std::vector<ivec2> points;

            for (auto& b : tresult)
            {
                if (&g == &b) { continue; }
                const auto p = b.to_points();
                points.insert(points.begin(), p.begin(), p.end());
            }

            const auto p = g.to_points();

            for (auto& gp : p)
            {
                bool is_not_in = true;
                for (auto& bp : points)
                {
                    if (gp.x == bp.x && gp.y == bp.y)
                    {
                        is_not_in = false;
                    }
                }
                if (is_not_in) { result.push_back(g); break; }
            }
        }

        return result;
    }

    // this will check the uniformity of all the values in a group
    // 1 1              1 0
    // 1 1              1 1
    // result = true    result = false
    bool kmap::check_group(const kgroup& group, const bool v) const
    {
        for (auto i = group.start.x; i < group.start.x + group.size.x; ++i)
        {
            for (auto j = group.start.y; j < group.start.y + group.size.y; ++j)
            {
                auto fv = this->value_for(i, j);
                if (fv) { if (*fv != v) { return false; } }
            }
        }
        return true;
    }

    // returns a list of coordinates from a kgroup
    // this loops through all the coordinates in a specific group
    std::vector<var_coord_t const*> kmap::coords_from_kgroup(const kgroup& group) const
    {
        std::vector<var_coord_t const*> result;
        for (auto i = group.start.x; i < group.start.x + group.size.x; ++i)
        {
            for (auto j = group.start.y; j < group.start.y + group.size.y; ++j)
            {
                auto fv = this->var_coord_for(i, j);
                if (fv) { result.push_back(fv); }
            }
        }
        return result;
    }

    // returns the solved formula string
    // v = true  -> SOP
    // v = false -> POS
    std::string kmap::get_formula_string(const bool v) const
    {
        std::stringstream ss;
        for (auto& m : this->get_formula(v))
        {
            if (!ss.str().empty()) { ss << ' ' << (v ? '+' : 'x') << ' '; };
            std::stringstream ssn;
            for (auto& [k, vv] : m)
            {
                if (!ssn.str().empty() && vv != karnaugh::k_var_result::k_vnull) { ssn << ' ' << (!v ? '+' : 'x') << ' '; }
                if (vv == karnaugh::k_var_result::k_vfalse) { ssn << '!'; }
                if (vv != karnaugh::k_var_result::k_vnull) { ssn << k; }
            }
            ss << '(' << ssn.str() << ')';
        }
        return ss.str();
    }

    // gets the formula of the karnaugh map
    // every group will be checked whether it has 
    // constant variables
    // if it does, it will compare it to the actual
    // value of the cell
    std::vector<std::map<char, k_var_result>> kmap::get_formula(const bool v) const
    {
        std::vector<std::map<char, k_var_result>> result;
        for (auto& g : this->get_filtered_groups(v))
        {
            auto map = utils::map_from_keys<std::map<char, k_var_result>>(this->unordered_variables);
            for (auto& var : this->unordered_variables)
            {
                bool* cvalue = nullptr;
                bool* value = nullptr;
                bool is_different = false;
                for (auto& c : this->coords_from_kgroup(g))
                {
                    if (!cvalue) { cvalue = const_cast<bool*>(this->value_for(c->first.second, c->second.second)); }
                    const bool* nvalue = nullptr;
                    try { nvalue = &c->first.first.at(var); }
                    catch (const std::exception& e) {}
                    try { nvalue = &c->second.first.at(var); }
                    catch (const std::exception& e) {}
                    if (!nvalue) { is_different = true; break; }
                    else if (value == nullptr) { value = const_cast<bool*>(nvalue); }
                    else if (*value != *nvalue) { is_different = true; break; }
                }
                if (is_different || !cvalue || !value) { map.at(var) = k_var_result::k_vnull; }
                else { map.at(var) = (*value == *cvalue) ? k_var_result::k_vtrue : k_var_result::k_vfalse; }
            }
            result.push_back(map);
        }
        return result;
    }
}