#include <ul/menu/ui/ui_EntryMenu.hpp>
#include <ul/menu/ui/ui_MainMenuLayout.hpp>
#include <ul/menu/ui/ui_MenuApplication.hpp>
#include <ul/menu/smi/smi_Commands.hpp>
#include <ul/util/util_String.hpp>
#include <ul/acc/acc_Accounts.hpp>

extern ul::menu::ui::MenuApplication::Ref g_MenuApplication;
extern ul::cfg::Config g_Config;

namespace ul::menu::ui {

    namespace {

        u32 g_EntryHeightCount = 0;

        void SetEntryHeightCount(const u32 count) {
            UL_ASSERT_TRUE(g_Config.SetEntry(cfg::ConfigEntryId::MenuEntryHeightCount, static_cast<u64>(count)));
            SaveConfig();

            g_EntryHeightCount = count;
        }

    }

    void EntryMenu::SwipeSingleLeft() {
        const auto old_entry_idx = this->cur_entry_idx;
        this->base_entry_idx_x--;
        this->cur_entry_idx -= g_EntryHeightCount;
        this->cur_entry_change_started_cb();
        this->NotifyFocusedEntryChanged(old_entry_idx);
    }

    void EntryMenu::SwipeSingleRight() {
        const auto old_entry_idx = this->cur_entry_idx;
        this->base_entry_idx_x++;
        this->cur_entry_idx += g_EntryHeightCount;
        this->cur_entry_change_started_cb();
        this->NotifyFocusedEntryChanged(old_entry_idx);
    }

    void EntryMenu::SwipeToPreviousPage() {
        const auto old_entry_idx = this->cur_entry_idx;
        const auto cur_idx_x = this->cur_entry_idx / g_EntryHeightCount;
        const auto cur_idx_y = this->cur_entry_idx % g_EntryHeightCount;

        if(cur_idx_x > 0) {
            if(this->base_entry_idx_x < this->entry_width_count) {
                this->base_entry_idx_x = 0;
            }
            else {
                this->base_entry_idx_x -= this->entry_width_count;
            }
            
            if(this->cur_entry_idx < this->entry_page_count) {
                this->cur_entry_idx = cur_idx_y;
            }
            else {
                this->cur_entry_idx -= this->entry_page_count;
            }

            this->NotifyFocusedEntryChanged(old_entry_idx);
        }
    }

    void EntryMenu::SwipeToNextPage() {
        const auto old_entry_idx = this->cur_entry_idx;
        this->base_entry_idx_x += this->entry_width_count;
        this->cur_entry_idx += this->entry_width_count * g_EntryHeightCount;
        this->NotifyFocusedEntryChanged(old_entry_idx);
    }

    void EntryMenu::SwipeRewind() {
        const auto old_entry_idx = this->cur_entry_idx;
        this->base_entry_idx_x = 0;
        this->cur_entry_idx = 0;

        this->NotifyFocusedEntryChanged(old_entry_idx);
    }

    pu::sdl2::TextureHandle::Ref EntryMenu::LoadEntryIconTexture(const Entry &entry) {
        auto icon_path = entry.control.icon_path;
        if(icon_path.empty()) {
            if(entry.Is<EntryType::Application>()) {
                icon_path = "ui/DefaultApplicationEntryIcon";
            }
            else if(entry.Is<EntryType::Homebrew>()) {
                icon_path = "ui/DefaultHomebrewEntryIcon";
            }
            else if(entry.Is<EntryType::Folder>()) {
                return this->folder_entry_icon;
            }
            else if(entry.Is<EntryType::SpecialEntryMiiEdit>()) {
                icon_path = "ui/MiiEditEntryIcon";
            }
            else if(entry.Is<EntryType::SpecialEntryWebBrowser>()) {
                icon_path = "ui/WebBrowserEntryIcon";
            }
            else if(entry.Is<EntryType::SpecialEntryUserPage>()) {
                // Not a theme asset
                return GetSelectedUserIconTexture();
            }
            else if(entry.Is<EntryType::SpecialEntrySettings>()) {
                icon_path = "ui/SettingsEntryIcon";
            }
            else if(entry.Is<EntryType::SpecialEntryThemes>()) {
                icon_path = "ui/ThemesEntryIcon";
            }
            else if(entry.Is<EntryType::SpecialEntryControllers>()) {
                icon_path = "ui/ControllersEntryIcon";
            }
            else if(entry.Is<EntryType::SpecialEntryAlbum>()) {
                icon_path = "ui/AlbumEntryIcon";
            }
            return TryFindLoadImageHandle(icon_path);
        }
        else {
            return pu::sdl2::TextureHandle::New(pu::ui::render::LoadImage(icon_path));
        }
    }

    bool EntryMenu::LoadEntry(const u32 idx) {
        UL_LOG_WARN("Loading entry %d", idx);
        if(idx < this->cur_entries.size()) {
            const auto &entry = this->cur_entries.at(idx);
            if(!entry.Is<EntryType::Invalid>()) {
                this->entry_icons.at(idx) = this->LoadEntryIconTexture(entry);

                auto &entry_alpha = this->load_img_entry_alphas.at(idx);
                entry_alpha = 0;
                auto &entry_alpha_incr = this->load_img_entry_incrs.at(idx);
                entry_alpha_incr.StartFromZero(EntryShowIncrementSteps, 0xFF);
                return true;
            }
        }

        return false;
    }

    void EntryMenu::NotifyFocusedEntryChanged(const s32 prev_entry_idx, const s32 is_prev_entry_suspended_override) {
        auto has_prev_entry = prev_entry_idx >= 0;
        bool is_prev_entry_suspended = false;
        if(is_prev_entry_suspended_override >= 0) {
            has_prev_entry = true;
            is_prev_entry_suspended = static_cast<bool>(is_prev_entry_suspended_override);
        }
        else if(prev_entry_idx >= 0) {
            if(((u32)prev_entry_idx < this->cur_entries.size())) {
                const auto &prev_entry = this->cur_entries.at(prev_entry_idx);
                if(prev_entry.type != EntryType::Invalid) {
                    is_prev_entry_suspended = g_MenuApplication->IsEntrySuspended(prev_entry);
                }
            }
        }

        const auto is_cur_entry_suspended = this->IsFocusedNonemptyEntry() && g_MenuApplication->IsEntrySuspended(this->GetFocusedEntry());
        this->cur_entry_changed_cb(has_prev_entry, is_prev_entry_suspended, is_cur_entry_suspended);
    }

    void EntryMenu::SetFocusedEntryIndex(const u32 idx) {
        const auto idx_x = idx / g_EntryHeightCount;

        // Set base at the start of the corresponding page
        this->base_entry_idx_x = (idx_x / this->entry_width_count) * this->entry_width_count;
        this->cur_entry_idx = idx;
    }

    Entry &EntryMenu::GetEntry(const s32 index) {
        UL_ASSERT_TRUE(index >= 0);
        UL_ASSERT_TRUE((u32)index < this->cur_entries.size());

        auto &entry = this->cur_entries.at(index);
        UL_ASSERT_TRUE(!entry.Is<EntryType::Invalid>());

        return entry;
    }

    EntryMenu::EntryMenu(const s32 x, const s32 y, const std::string &path, FocusedEntryInputPressedCallback cur_entry_input_cb, FocusedEntryChangedCallback cur_entry_changed_cb, FocusedEntryChangeStartedCallback cur_entry_change_started_cb) : x(x), y(y), cur_entry_idx(0), entry_idx_stack(), cur_entry_input_cb(cur_entry_input_cb), cur_entry_changed_cb(cur_entry_changed_cb), cur_entry_change_started_cb(cur_entry_change_started_cb), enabled(true), selected_entry_idx(-1), empty_entry_icon(nullptr), selected_entry_icon(nullptr), entry_h_margin(EntryVerticalMargin), entries_to_add(), entries_to_remove(), pending_load_img_entry_start_idx(UINT32_MAX), pending_load_img_entry_ext_idx(UINT32_MAX), pending_load_img_done(false), entry_page_count(0), entry_total_count(0), swipe_mode(SwipeMode::None), entries_base_swipe_neg_offset(0), entries_base_swipe_neg_offset_incr(), extra_entry_swipe_show_h_count(0) {
        this->cursor_over_icon = TryFindLoadImage("ui/CursorOverIcon");
        this->cursor_size = 0;
        this->border_over_icon = TryFindLoadImage("ui/BorderOverIcon");
        this->border_size = 0;
        this->suspended_over_icon = TryFindLoadImage("ui/SuspendedOverIcon");
        this->suspended_size = 0;
        this->selected_over_icon = TryFindLoadImage("ui/SelectedOverIcon");
        this->selected_size = 0;
        this->hb_takeover_app_over_icon = TryFindLoadImage("ui/HomebrewTakeoverApplicationOverIcon");
        this->hb_takeover_app_size = 0;

        this->empty_entry_icon = TryFindLoadImageHandle("ui/EmptyEntryIcon");
        this->folder_entry_icon = TryFindLoadImageHandle("ui/FolderEntryIcon");

        this->cur_path = path;

        this->base_entry_idx_x = 0;
        this->cursor_transition_x = 0;
        this->cursor_transition_y = 0;
        this->cursor_transition_x_incr = {};
        this->cursor_transition_y_incr = {};
        this->after_transition_base_entry_idx_x = -1;
        this->after_transition_entry_idx = -1;
    }

    void EntryMenu::Initialize(const u32 last_idx) {
        u64 entry_height_count;
        UL_ASSERT_TRUE(g_Config.GetEntry(cfg::ConfigEntryId::MenuEntryHeightCount, entry_height_count));
        g_EntryHeightCount = entry_height_count;
        this->ComputeSizes(g_EntryHeightCount);

        this->entry_idx_stack.push(last_idx);
        this->MoveTo("", true);
    }

    void EntryMenu::OnRender(pu::ui::render::Renderer::Ref &drawer, const s32 x, const s32 y) {
        const auto loaded_pending_load_img_entry_ext_idx = this->pending_load_img_entry_ext_idx;
        if(!this->pending_load_img_done) {
            const auto has_left = this->pending_load_img_entry_start_idx >= this->pending_load_img_entry_ext_idx;
            if(has_left) {
                this->LoadEntry(this->pending_load_img_entry_start_idx - this->pending_load_img_entry_ext_idx);
            }

            const auto right_load_idx = this->pending_load_img_entry_start_idx + this->pending_load_img_entry_ext_idx;
            const auto has_right = right_load_idx < this->cur_entries.size();
            if(has_right) {
                this->LoadEntry(right_load_idx);
            }

            if(!has_left && !has_right) {
                this->pending_load_img_done = true;
                this->pending_load_img_entry_ext_idx = UINT32_MAX;
            }
            else {
                this->pending_load_img_entry_ext_idx++;
            }
        }

        if(this->entries_base_swipe_neg_offset_incr.Increment(this->entries_base_swipe_neg_offset)) {
            this->entries_base_swipe_neg_offset = 0;
            switch(this->swipe_mode) {
                case SwipeMode::LeftSingle: {
                    break;
                }
                case SwipeMode::RightSingle: {
                    this->SwipeSingleRight();
                    break;
                }
                case SwipeMode::LeftPage: {
                    break;
                }
                case SwipeMode::RightPage: {
                    this->SwipeToNextPage();
                    break;
                }
                case SwipeMode::Rewind: {
                    break;
                }
                default:
                    break;
            }
            this->swipe_mode = SwipeMode::None;
            this->extra_entry_swipe_show_h_count = 0;
        }

        const auto cur_actual_entry_idx = this->IsCursorInTransition() ? this->pre_transition_entry_idx : this->cur_entry_idx;
        const auto base_idx = this->base_entry_idx_x * g_EntryHeightCount;

        // For the swipe animations, additional items are rendered (some will be outside the visible range)
        const auto entry_render_count = this->entry_page_count + this->extra_entry_swipe_show_h_count * g_EntryHeightCount;
        for(u32 i = 0; i < entry_render_count; i++) {
            const auto entry_i = base_idx + i;

            pu::sdl2::TextureHandle::Ref entry_icon = {};
            if(entry_i < this->entry_icons.size()) {
                entry_icon = this->entry_icons.at(entry_i);
            }

            const auto is_empty = (entry_i >= this->entry_icons.size()) || (entry_i >= this->cur_entries.size()) || ((entry_i < this->cur_entries.size()) && this->cur_entries.at(entry_i).Is<EntryType::Invalid>());

            const auto idx_x = i / g_EntryHeightCount;
            const auto idx_y = i % g_EntryHeightCount;
            const auto entry_x = x + HorizontalSideMargin + idx_x * (this->entry_size + this->entry_h_margin);
            const auto entry_y = y + this->v_side_margin + idx_y * (this->entry_size + EntryVerticalMargin);

            drawer->RenderTexture(this->empty_entry_icon->Get(), (s32)entry_x - (s32)this->entries_base_swipe_neg_offset, entry_y, pu::ui::render::TextureRenderOptions::WithCustomDimensions(this->entry_size, this->entry_size));

            if(!is_empty) {
                const auto &entry = this->cur_entries.at(entry_i);
                auto &entry_alpha = this->load_img_entry_alphas.at(entry_i);
                auto &entry_alpha_incr = this->load_img_entry_incrs.at(entry_i);
                entry_alpha_incr.Increment(entry_alpha);

                if(entry_icon != nullptr) {
                    drawer->RenderTexture(entry_icon->Get(), (s32)entry_x - (s32)this->entries_base_swipe_neg_offset, entry_y, pu::ui::render::TextureRenderOptions::WithCustomAlphaAndDimensions(entry_alpha, this->entry_size, this->entry_size));
                }

                if(entry.Is<EntryType::Homebrew>() || entry.Is<EntryType::Application>() || entry.IsSpecial()) {
                    // Only for valid non-folder entries
                    const auto border_x = entry_x - ((this->border_size - this->entry_size) / 2);
                    const auto border_y = entry_y - ((this->border_size - this->entry_size) / 2);
                    drawer->RenderTexture(this->border_over_icon, (s32)border_x - (s32)this->entries_base_swipe_neg_offset, border_y, pu::ui::render::TextureRenderOptions::WithCustomDimensions(this->border_size, this->border_size));
                }

                if(g_MenuApplication->IsEntryHomebrewTakeoverApplication(entry)) {
                    const auto takeover_x = (s32)(entry_x - ((this->hb_takeover_app_size - this->entry_size) / 2)) - (s32)this->entries_base_swipe_neg_offset;
                    const auto takeover_y = entry_y - ((this->hb_takeover_app_size - this->entry_size) / 2);
                    drawer->RenderTexture(this->hb_takeover_app_over_icon, takeover_x, takeover_y, pu::ui::render::TextureRenderOptions::WithCustomDimensions(this->hb_takeover_app_size, this->hb_takeover_app_size));
                }

                if(g_MenuApplication->IsEntrySuspended(entry)) {
                    const auto suspended_x = (s32)(entry_x - ((this->suspended_size - this->entry_size) / 2)) - (s32)this->entries_base_swipe_neg_offset;
                    const auto suspended_y = entry_y - ((this->suspended_size - this->entry_size) / 2);
                    drawer->RenderTexture(this->suspended_over_icon, suspended_x, suspended_y, pu::ui::render::TextureRenderOptions::WithCustomDimensions(this->suspended_size, this->suspended_size));
                }

                if(this->IsEntrySelected(entry_i)) {
                    const auto selected_x = (s32)(entry_x - ((this->selected_size - this->entry_size) / 2)) - (s32)this->entries_base_swipe_neg_offset;
                    const auto selected_y = entry_y - ((this->selected_size - this->entry_size) / 2);
                    drawer->RenderTexture(this->selected_over_icon, selected_x, selected_y, pu::ui::render::TextureRenderOptions::WithCustomDimensions(this->selected_size, this->selected_size));
                }
            }

            if(loaded_pending_load_img_entry_ext_idx != UINT32_MAX) {
                if((this->pending_load_img_entry_start_idx + loaded_pending_load_img_entry_ext_idx) == this->cur_entry_idx) {
                    this->NotifyFocusedEntryChanged(-1);
                }
                if((this->pending_load_img_entry_start_idx >= loaded_pending_load_img_entry_ext_idx) && ((this->pending_load_img_entry_start_idx - loaded_pending_load_img_entry_ext_idx) == this->cur_entry_idx)) {
                    this->NotifyFocusedEntryChanged(-1);
                }
            }
        }

        const auto cur_actual_entry_idx_x = (cur_actual_entry_idx / g_EntryHeightCount) - this->base_entry_idx_x;
        const auto cur_actual_entry_idx_y = cur_actual_entry_idx % g_EntryHeightCount;

        const auto cursor_x = x + HorizontalSideMargin + cur_actual_entry_idx_x * (this->entry_size + this->entry_h_margin) - ((this->cursor_size - this->entry_size) / 2) + (this->IsCursorInTransition() ? this->cursor_transition_x : 0);
        const auto cursor_y = y + this->v_side_margin + cur_actual_entry_idx_y * (this->entry_size + EntryVerticalMargin) - ((this->cursor_size - this->entry_size) / 2) + (this->IsCursorInTransition() ? this->cursor_transition_y : 0);
        drawer->RenderTexture(this->cursor_over_icon, cursor_x, cursor_y, pu::ui::render::TextureRenderOptions::WithCustomDimensions(this->cursor_size, this->cursor_size));

        if((this->selected_entry_idx >= 0) && (this->selected_entry_icon != nullptr)) {
            const auto entry_size_factor = (double)this->entry_size / (double)DefaultEntrySize;
            const auto over_icon_offset = (u32)(entry_size_factor * (double)DefaultMoveOverIconOffset);
            drawer->RenderTexture(this->selected_entry_icon->Get(), cursor_x - over_icon_offset, cursor_y - over_icon_offset, pu::ui::render::TextureRenderOptions::WithCustomAlphaAndDimensions(MoveOverIconAlpha, this->entry_size, this->entry_size));
        }

        if(this->IsCursorInTransition()) {
            if(this->cursor_transition_x_incr.Increment(this->cursor_transition_x) || this->cursor_transition_y_incr.Increment(this->cursor_transition_y)) {
                this->cursor_transition_x = 0;
                this->cursor_transition_y = 0;

                if(this->after_transition_base_entry_idx_x >= 0) {
                    this->base_entry_idx_x = this->after_transition_base_entry_idx_x;
                    this->NotifyFocusedEntryChanged(this->pre_transition_entry_idx);
                }

                if(this->after_transition_entry_idx >= 0) {
                    this->cur_entry_idx = this->after_transition_entry_idx;
                    this->NotifyFocusedEntryChanged(this->pre_transition_entry_idx);
                }

                this->after_transition_base_entry_idx_x = -1;
                this->after_transition_entry_idx = -1;
            }
        }
    }

    void EntryMenu::OnInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) {
        if(this->IsCursorInTransition()) {
            return;
        }
        if(!this->enabled) {
            return;
        }

        const auto cursor_transition_abs_incr_x = this->entry_h_margin + this->entry_size;
        const auto cursor_transition_abs_incr_y = EntryVerticalMargin + this->entry_size;

        if((keys_down & HidNpadButton_Up) || (keys_held & HidNpadButton_StickLUp) || (keys_held & HidNpadButton_StickRUp)) {
            const auto cur_entry_idx_y = this->cur_entry_idx % g_EntryHeightCount;

            if(cur_entry_idx_y > 0) {
                this->pre_transition_entry_idx = this->cur_entry_idx;
                this->after_transition_entry_idx = this->cur_entry_idx - 1;
                this->cursor_transition_y_incr.Start(CursorTransitionIncrementSteps, 0, -cursor_transition_abs_incr_y);
                this->cur_entry_change_started_cb();
            }
        }
        else if((keys_down & HidNpadButton_Down) || (keys_held & HidNpadButton_StickLDown) || (keys_held & HidNpadButton_StickRDown)) {
            const auto cur_entry_idx_y = this->cur_entry_idx % g_EntryHeightCount;
            if((cur_entry_idx_y + 1) < g_EntryHeightCount) {
                this->pre_transition_entry_idx = this->cur_entry_idx;
                this->after_transition_entry_idx = this->cur_entry_idx + 1;
                this->cursor_transition_y_incr.Start(CursorTransitionIncrementSteps, 0, cursor_transition_abs_incr_y);
                this->cur_entry_change_started_cb();
            }
        }
        else if((keys_down & HidNpadButton_Left) || (keys_held & HidNpadButton_StickLLeft) || (keys_held & HidNpadButton_StickRLeft)) {
            const auto rel_entry_idx_x = (this->cur_entry_idx / g_EntryHeightCount) - this->base_entry_idx_x;
            if(rel_entry_idx_x > 0) {
                this->pre_transition_entry_idx = this->cur_entry_idx;
                this->after_transition_entry_idx = this->cur_entry_idx - g_EntryHeightCount;
                this->cursor_transition_x_incr.Start(CursorTransitionIncrementSteps, 0, -cursor_transition_abs_incr_x);
                this->cur_entry_change_started_cb();
            }
            else if((rel_entry_idx_x == 0) && (this->base_entry_idx_x > 0)) {
                if(this->IsNotInSwipe()) {
                    this->SwipeSingleLeft();

                    this->entries_base_swipe_neg_offset = this->entry_size + this->entry_h_margin /* x1 */;
                    this->entries_base_swipe_neg_offset_incr.StartToZero(EntriesSwipeSingleIncrementSteps, this->entries_base_swipe_neg_offset);
                    this->swipe_mode = SwipeMode::LeftSingle;
                }
            }
        }
        else if((keys_down & HidNpadButton_Right) || (keys_held & HidNpadButton_StickLRight) || (keys_held & HidNpadButton_StickRRight)) {
            const auto rel_entry_idx_x = (this->cur_entry_idx / g_EntryHeightCount) - this->base_entry_idx_x;
            if((rel_entry_idx_x + 1) < this->entry_width_count) {
                this->pre_transition_entry_idx = this->cur_entry_idx;
                this->after_transition_entry_idx = this->cur_entry_idx + g_EntryHeightCount;
                this->cursor_transition_x_incr.Start(CursorTransitionIncrementSteps, 0, cursor_transition_abs_incr_x);
                this->cur_entry_change_started_cb();
            }
            else if((rel_entry_idx_x + 1) == this->entry_width_count) {
                if(this->IsNotInSwipe()) {
                    this->entries_base_swipe_neg_offset = 0;
                    this->entries_base_swipe_neg_offset_incr.StartFromZero(EntriesSwipeSingleIncrementSteps, this->entry_size + this->entry_h_margin /* x1 */);
                    this->swipe_mode = SwipeMode::RightSingle;  
                }
            }
        }
        else {
            this->cur_entry_input_cb(keys_down);
        }
    }

    void EntryMenu::MoveTo(const std::string &new_path, const bool force_pop_idx) {
        const auto reloading = new_path.empty();
        const auto going_back = new_path.length() < this->cur_path.length();
        const auto old_entry_idx = this->cur_entry_idx;

        s32 is_prev_entry_suspended_override = -1;
        if(!reloading) {
            if(this->IsFocusedNonemptyEntry()) {
                is_prev_entry_suspended_override = static_cast<s32>(g_MenuApplication->IsEntrySuspended(this->GetFocusedEntry()));
            }

            this->cur_path = new_path;
            this->base_entry_idx_x = 0;
            this->cur_entry_idx = 0;
        }

        if((going_back && !this->entry_idx_stack.empty()) || force_pop_idx) {
            const auto idx = this->entry_idx_stack.top();
            this->SetFocusedEntryIndex(idx);
            this->entry_idx_stack.pop();
        }

        this->cur_entries.clear();
        this->cur_entries = LoadEntries(this->cur_path);
        this->OrganizeEntries();
        this->ComputeSizes(g_EntryHeightCount);
        
        for(auto &entry_icon : this->entry_icons) {
            if(entry_icon != this->selected_entry_icon) {
                entry_icon = {};
            }
        }

        this->entry_icons.clear();
        this->load_img_entry_alphas.clear();
        this->load_img_entry_incrs.clear();
        for(u32 i = 0; i < this->entry_total_count; i++) {
            this->entry_icons.push_back(nullptr);
            this->load_img_entry_alphas.push_back(0xFF);
            this->load_img_entry_incrs.push_back({});
        }

        this->pending_load_img_entry_start_idx = this->cur_entry_idx;
        this->LoadEntry(this->cur_entry_idx);
        this->NotifyFocusedEntryChanged(-1, is_prev_entry_suspended_override);
        this->pending_load_img_entry_ext_idx = 1;
        this->pending_load_img_done = false;

        if(!reloading && !going_back) {
            this->entry_idx_stack.push(old_entry_idx);
        }
    }

    void EntryMenu::OrganizeEntries() {
        std::vector<Entry> tmp_entries;
        
        for(const auto &entry: this->cur_entries) {
            if(!entry.Is<EntryType::Invalid>()) {
                while(tmp_entries.size() < (entry.index + 1)) {
                    auto &tmp_entry = tmp_entries.emplace_back();
                    tmp_entry.type = EntryType::Invalid;
                }

                tmp_entries.at(entry.index) = entry;
            }
        }
        
        for(const auto &rem_entry: this->entries_to_remove) {
            UL_LOG_WARN("Removing entry of type %d at %d", (u32)rem_entry.type, rem_entry.index);
            // Otherwise no need to add an entry at all
            if(rem_entry.index < tmp_entries.size()) {
                auto &tmp_entry = tmp_entries.at(rem_entry.index);
                tmp_entry.type = EntryType::Invalid;
            }
            this->entry_icons.at(rem_entry.index) = {};
        }

        for(const auto &add_entry: this->entries_to_add) {
            UL_LOG_WARN("Adding entry of type %d at %d", (u32)add_entry.type, add_entry.index);
            if(add_entry.type != EntryType::Invalid) {
                while(tmp_entries.size() < (add_entry.index + 1)) {
                    UL_LOG_WARN("pushpush");
                    auto &tmp_entry = tmp_entries.emplace_back();
                    tmp_entry.type = EntryType::Invalid;
                }

                tmp_entries.at(add_entry.index) = add_entry;
            }
        }

        std::swap(this->cur_entries, tmp_entries);
        tmp_entries.clear();

        this->ComputeSizes(g_EntryHeightCount);
        while(this->entry_icons.size() < this->entry_total_count) {
            this->entry_icons.push_back(nullptr);
            this->load_img_entry_alphas.push_back(0xFF);
            this->load_img_entry_incrs.push_back({});
        }

        for(const auto &add_entry: this->entries_to_add) {
            this->LoadEntry(add_entry.index);
        }

        this->entries_to_add.clear();
        this->entries_to_remove.clear();
    }

    void EntryMenu::OrganizeUpdateEntries(const s32 is_prev_entry_suspended_override) {
        this->OrganizeEntries();
        this->NotifyFocusedEntryChanged(-1, is_prev_entry_suspended_override);
    }

    void EntryMenu::NotifyEntryAdded(const Entry &entry) {
        this->entries_to_add.push_back(entry);
    }

    void EntryMenu::NotifyEntryRemoved(const Entry &entry) {
        UL_ASSERT_TRUE(entry.index < this->cur_entries.size());

        this->entries_to_remove.push_back(entry);
    }

    bool EntryMenu::IsFocusedNonemptyEntry() {
        return (this->cur_entry_idx < this->cur_entries.size()) && !this->cur_entries.at(this->cur_entry_idx).Is<EntryType::Invalid>();
    }

    void EntryMenu::IncrementEntryHeightCount() {
        auto h_count = g_EntryHeightCount;
        h_count++;

        g_MenuApplication->SetBackgroundFade();
        g_MenuApplication->FadeOut();
        if(!this->entry_idx_stack.empty()) {
            this->entry_idx_stack.top() = 0;
        }
        SetEntryHeightCount(h_count);
        this->ComputeSizes(h_count);

        this->base_entry_idx_x = 0;
        this->cur_entry_idx = 0;

        this->NotifyFocusedEntryChanged(-1);
        g_MenuApplication->FadeIn();
    }
    
    void EntryMenu::DecrementEntryHeightCount() {
        auto h_count = g_EntryHeightCount;
        if(h_count > 1) {
            h_count--;

            g_MenuApplication->SetBackgroundFade();
            g_MenuApplication->FadeOut();
            this->ComputeSizes(h_count);

            const auto new_count = h_count * this->entry_width_count;
            if(new_count == 0) {
                h_count++;
                this->ComputeSizes(h_count);
            }
            else {
                if(!this->entry_idx_stack.empty()) {
                    this->entry_idx_stack.top() = 0;
                }
                SetEntryHeightCount(h_count);

                this->base_entry_idx_x = 0;
                this->cur_entry_idx = 0;

                this->NotifyFocusedEntryChanged(-1);
            }
            g_MenuApplication->FadeIn();
        }
    }

    void EntryMenu::MoveToPreviousPage() {
        if((this->base_entry_idx_x > 0) && this->IsNotInSwipe()) {
            const auto prev_base_entry_idx_x = this->base_entry_idx_x;
            this->SwipeToPreviousPage();
            this->extra_entry_swipe_show_h_count = std::min(this->entry_width_count, prev_base_entry_idx_x);
            this->entries_base_swipe_neg_offset = (this->entry_size + this->entry_h_margin) * this->extra_entry_swipe_show_h_count;
            this->entries_base_swipe_neg_offset_incr.StartToZero(EntriesSwipePageIncrementSteps, this->entries_base_swipe_neg_offset);
            this->swipe_mode = SwipeMode::LeftPage;
        }
    }

    void EntryMenu::MoveToNextPage() {
        if(this->IsNotInSwipe()) {
            this->extra_entry_swipe_show_h_count = this->entry_width_count;
            this->entries_base_swipe_neg_offset = 0;
            this->entries_base_swipe_neg_offset_incr.StartFromZero(EntriesSwipePageIncrementSteps, (this->entry_size + this->entry_h_margin) * this->extra_entry_swipe_show_h_count);
            this->swipe_mode = SwipeMode::RightPage;  
        }
    }

    void EntryMenu::Rewind() {
        if((this->cur_entry_idx > 0) && this->IsNotInSwipe()) {
            const auto prev_base_entry_idx_x = this->base_entry_idx_x;
            this->SwipeRewind();
            if(prev_base_entry_idx_x > 0) {
                // No need to animate anything if we're already in the first page
                this->extra_entry_swipe_show_h_count = prev_base_entry_idx_x;
                this->entries_base_swipe_neg_offset = (this->entry_size + this->entry_h_margin) * this->extra_entry_swipe_show_h_count;
                this->entries_base_swipe_neg_offset_incr.StartToZero(EntriesSwipeRewindIncrementSteps, this->entries_base_swipe_neg_offset);
                this->swipe_mode = SwipeMode::Rewind;
            }
        }
    }

    void EntryMenu::ResetSelection() {
        this->selected_entry_idx = -1;
        this->selected_entry_icon = nullptr;
    }

    void EntryMenu::SetEntrySelected(const u32 idx, const bool selected) {
        this->ResetSelection();
        if(selected) {
            UL_ASSERT_TRUE(idx < this->cur_entries.size());
            this->selected_entry_idx = idx;
            this->selected_entry_icon = this->entry_icons.at(idx);
        }
    }

    bool EntryMenu::IsEntrySelected(const u32 idx) {
        return this->selected_entry_idx == (s32)idx;
    }

    bool EntryMenu::IsAnySelected() {
        return this->selected_entry_idx != -1;
    }

    bool EntryMenu::IsFocusedEntrySuspended() {
        if(!this->IsFocusedNonemptyEntry()) {
            return false;
        }
        else {
            return g_MenuApplication->IsEntrySuspended(this->GetFocusedEntry());
        }
    }

}
