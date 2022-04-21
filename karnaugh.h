#pragma once

#include <map>
#include <unordered_map>
#include <filesystem>

namespace karnaugh
{
    using var_map_t   = std::map<char, bool>;
    using var_pmap_t  = std::pair<var_map_t, int>;
    using var_list_t  = std::vector<char>;
    using var_coord_t = std::pair<var_pmap_t, var_pmap_t>;

    enum class k_var_result
    {
        k_vtrue, k_vfalse, k_vnull
    };

    struct ivec2
    {
        int x = 0, y = 0;
        ivec2() { x = y = 0; }
        ivec2(int _x, int _y) : x(_x), y(_y) {};
    };

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
        var_list_t unordered_variables;
        std::pair<var_list_t, var_list_t> variables;

        kmap(const std::filesystem::path& path);

        size_t size_x() const { return 1 << this->variables.first.size();  }
        size_t size_y() const { return 1 << this->variables.second.size(); }
        size_t size()   const { return this->size_x() * this->size_y();    }
        bool   empty()  const { return this->size() == 0u;                 }

        const bool*        value_for     (const int x, const int y) const;
        const var_coord_t* var_coord_for (const int x, const int y) const;
        const var_map_t*   var_map_for_x (const int x) const;
        const var_map_t*   var_map_for_y (const int y) const;

        std::vector<kgroup> get_all_groups      (const bool v) const;
        std::vector<kgroup> get_filtered_groups (const bool v) const;

        std::vector<std::map<char, k_var_result>> get_formula(const bool v) const;
        std::string get_formula_string(const bool v) const;

    private:
        std::vector<std::pair<var_coord_t, bool>> internal_map;

        void load_variables(std::istringstream& iss);
        void load_values(std::istringstream& iss);

        bool check_group(const kgroup& group, const bool v) const;
        std::vector<var_coord_t const*> coords_from_kgroup(const kgroup& group) const;
    };
}