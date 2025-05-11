
// Grabbed from Goldleaf's source code, slightly edited to work here

#include <ul/menu/ui/ui_ClickableImage.hpp>
#include <ul/fs/fs_Stdio.hpp>

namespace ul::menu::ui {

    ClickableImage::~ClickableImage() {
        this->img_tex = {};
    }

    void ClickableImage::SetImage(pu::sdl2::TextureHandle::Ref img) {
        this->img_tex = {};
        this->img_tex = img;
        this->w = pu::ui::render::GetTextureWidth(this->img_tex->Get());
        this->h = pu::ui::render::GetTextureHeight(this->img_tex->Get());
    }

    void ClickableImage::OnRender(pu::ui::render::Renderer::Ref &drawer, const s32 x, const s32 y) {
        drawer->RenderTexture(this->img_tex->Get(), x, y, pu::ui::render::TextureRenderOptions({}, this->w, this->h, {}, {}, {}));
    }

    void ClickableImage::OnInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) {
        if(this->touched) {
            const auto tp_now = std::chrono::steady_clock::now();
            const u64 diff = std::chrono::duration_cast<std::chrono::milliseconds>(tp_now - this->touch_tp).count();
            if(diff >= TouchActionTimeMs) {
                this->touched = false;
                if(this->cb) {
                    this->cb();
                }
                SDL_SetTextureColorMod(this->img_tex->Get(), 0xFF, 0xFF, 0xFF);
            }
        }
        else if(touch_pos.HitsRegion(this->GetProcessedX(), this->GetProcessedY(), this->w, this->h)) {
            this->touch_tp = std::chrono::steady_clock::now();
            this->touched = true;
            SDL_SetTextureColorMod(this->img_tex->Get(), RedColorMod, GreenColorMod, BlueColorMod);
        }
    }

}
