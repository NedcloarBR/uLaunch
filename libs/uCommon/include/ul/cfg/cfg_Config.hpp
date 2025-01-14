
#pragma once
#include <ul/ul_Include.hpp>
#include <ul/fs/fs_Stdio.hpp>
#include <ul/loader/loader_TargetTypes.hpp>
#include <ul/util/util_String.hpp>
#include <ul/util/util_Json.hpp>

namespace ul::cfg {

    struct ThemeManifest {
        std::string name;
        u32 format_version;
        std::string release;
        std::string description;
        std::string author;
    };

    struct Theme {
        std::string name;
        ThemeManifest manifest;

        inline bool IsValid() const {
            return !this->name.empty();
        }

        inline bool IsSame(const Theme &other) const {
            // Compare filenames, those must be unique
            return other.name == this->name;
        }
    };

    enum class ConfigEntryId : u8 {
        MenuTakeoverProgramId,
        HomebrewAppletTakeoverProgramId,
        HomebrewApplicationTakeoverApplicationId,
        ViewerUsbEnabled,
        ActiveThemeName,
        MenuEntryHeightCount
    };

    enum class ConfigEntryType : u8 {
        Bool,
        U64,
        String
    };

    struct ConfigEntryHeader {
        ConfigEntryId id;
        ConfigEntryType type;
        u8 size;
        u8 pad;
    };

    struct ConfigEntry {
        ConfigEntryHeader header;
        bool bool_value;
        u64 u64_value;
        std::string str_value;

        template<typename T>
        inline bool Get(T &out_t) const {
            switch(this->header.type) {
                case ConfigEntryType::Bool: {
                    if constexpr(std::is_same_v<T, bool>) {
                        out_t = this->bool_value;
                        return true;
                    }
                    else {
                        return false;
                    }
                }
                case ConfigEntryType::U64: {
                    if constexpr(std::is_same_v<T, u64>) {
                        out_t = this->u64_value;
                        return true;
                    }
                    else {
                        return false;
                    }
                }
                case ConfigEntryType::String: {
                    if constexpr(std::is_same_v<T, std::string>) {
                        out_t = this->str_value;
                        return true;
                    }
                    else {
                        return false;
                    }
                }
            }
            return false;
        }

        template<typename T>
        inline bool Set(const T &t) {
            switch(this->header.type) {
                case ConfigEntryType::Bool: {
                    if constexpr(std::is_same_v<T, bool>) {
                        this->bool_value = t;
                        return true;
                    }
                    else {
                        return false;
                    }
                }
                case ConfigEntryType::U64: {
                    if constexpr(std::is_same_v<T, u64>) {
                        this->u64_value = t;
                        return true;
                    }
                    else {
                        return false;
                    }
                }
                case ConfigEntryType::String: {
                    if constexpr(std::is_same_v<T, std::string>) {
                        this->str_value = t;
                        this->header.size = this->str_value.length();
                        return true;
                    }
                    else {
                        return false;
                    }
                }
            }
            return false;
        }
    };

    struct ConfigHeader {
        u32 magic;
        u32 entry_count;

        static constexpr u32 Magic = 0x47464355; // "UCFG"
    };

    struct Config {
        std::vector<ConfigEntry> entries;

        template<typename T>
        inline bool SetEntry(const ConfigEntryId id, const T &t) {
            for(auto &entry : this->entries) {
                if(entry.header.id == id) {
                    return entry.Set(t);
                }
            }
            // Create new entry
            ConfigEntry new_entry = {
                .header = {
                    .id = id
                }
            };
            switch(id) {
                case ConfigEntryId::MenuTakeoverProgramId:
                case ConfigEntryId::HomebrewAppletTakeoverProgramId:
                case ConfigEntryId::HomebrewApplicationTakeoverApplicationId: {
                    if constexpr(std::is_same_v<T, u64>) {
                        new_entry.header.type = ConfigEntryType::U64;
                        new_entry.header.size = sizeof(t);
                        new_entry.u64_value = t;
                        break;
                    }
                    else {
                        return false;
                    }
                }
                case ConfigEntryId::ViewerUsbEnabled: {
                    if constexpr(std::is_same_v<T, bool>) {
                        new_entry.header.type = ConfigEntryType::Bool;
                        new_entry.header.size = sizeof(t);
                        new_entry.bool_value = t;
                        break;
                    }
                    else {
                        return false;
                    }
                }
                case ConfigEntryId::ActiveThemeName: {
                    if constexpr(std::is_same_v<T, std::string>) {
                        new_entry.header.type = ConfigEntryType::String;
                        new_entry.header.size = t.length();
                        new_entry.str_value = t;
                        break;
                    }
                    else {
                        return false;
                    }
                }
                case ConfigEntryId::MenuEntryHeightCount: {
                    if constexpr(std::is_same_v<T, u64>) {
                        new_entry.header.type = ConfigEntryType::U64;
                        new_entry.header.size = sizeof(t);
                        new_entry.u64_value = t;
                        break;
                    }
                    else {
                        return false;
                    }
                }
            }
            this->entries.push_back(std::move(new_entry));
            return true;
        }
        
        template<typename T>
        inline bool GetEntry(const ConfigEntryId id, T &out_t) const {
            for(const auto &entry : this->entries) {
                if(entry.header.id == id) {
                    return entry.Get(out_t);
                }
            }

            // Default values
            switch(id) {
                case ConfigEntryId::MenuTakeoverProgramId: {
                    if constexpr(std::is_same_v<T, u64>) {
                        // Take over eShop by default
                        out_t = 0x010000000000100B;
                        return true;
                    }
                    else {
                        return false;
                    }
                }
                case ConfigEntryId::HomebrewAppletTakeoverProgramId: {
                    if constexpr(std::is_same_v<T, u64>) {
                        // Take over parental control applet by default
                        out_t = 0x0100000000001001;
                        return true;
                    }
                    else {
                        return false;
                    }
                }
                case ConfigEntryId::HomebrewApplicationTakeoverApplicationId: {
                    if constexpr(std::is_same_v<T, u64>) {
                        // No donor title by default
                        out_t = 0;
                        return true;
                    }
                    else {
                        return false;
                    }
                }
                case ConfigEntryId::ViewerUsbEnabled: {
                    if constexpr(std::is_same_v<T, bool>) {
                        // Disabled by default, it might interfer with other homebrews
                        out_t = false;
                        return true;
                    }
                    else {
                        return false;
                    }
                }
                case ConfigEntryId::ActiveThemeName: {
                    if constexpr(std::is_same_v<T, std::string>) {
                        // Empty by default
                        out_t = "";
                        return true;
                    }
                    else {
                        return false;
                    }
                }
                case ConfigEntryId::MenuEntryHeightCount: {
                    if constexpr(std::is_same_v<T, u64>) {
                        out_t = 3;
                        return true;
                    }
                    else {
                        return false;
                    }
                }
            }
            return false;
        }
    };

    constexpr u32 CurrentThemeFormatVersion = 2;

    inline bool IsThemeOutdated(const Theme &theme) {
        return theme.manifest.format_version < CurrentThemeFormatVersion;
    }

    Result TryLoadTheme(const std::string &theme_name, Theme &out_theme);
    Result TryCacheLoadThemeIcon(const Theme &theme, std::string &out_icon_path);

    std::vector<Theme> FindThemes();

    void EnsureCacheActiveTheme(const Config &cfg);
    void CacheActiveTheme(const Config &cfg);
    void RemoveActiveThemeCache();
    
    inline std::string GetActiveThemeResource(const std::string &resource_base) {
        return fs::JoinPath(ActiveThemeCachePath, resource_base);
    }

    void LoadLanguageJsons(const std::string &lang_base, util::JSON &lang, util::JSON &def);

    inline std::string GetLanguageString(const util::JSON &lang, const util::JSON &def, const std::string &name) {
        auto str = lang.value(name, "");
        if(str.empty()) {
            str = def.value(name, "");
        }
        return str;
    }

    Config CreateNewAndLoadConfig();
    Config LoadConfig();
    void SaveConfig(const Config &cfg);

}
