#include <ui.h>
#include <kernel.h>
#include <serial.h>
#include <png.h>
#include <rtc.h>
#include <mouse.h>
#include "font_data.h"

/* UI layout uses 1080x1920 design coordinates and scales to portrait VBE output. */
#define LOGICAL_W 1080
#define LOGICAL_H 1920
#define MAX_SCREEN_W 640
#define MAX_SCREEN_H 1136
static int screen_width=544, screen_height=960;
#define SCREEN_W screen_width
#define SCREEN_H screen_height
#define SYSTEM_TOP_END (96 * SCREEN_H / LOGICAL_H)
#define SYSTEM_BOTTOM_START (1860 * SCREEN_H / LOGICAL_H)
#define APP_COUNT 6
#define HOME_MAX_PAGES 4
#define RGB(r,g,b) (((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b))

#define C_BG RGB(14,19,35)
#define C_BG2 RGB(37,31,79)
#define C_WHITE RGB(249,249,255)
#define C_MUTED RGB(177,185,208)
#define C_CYAN RGB(111,225,232)
#define C_BLUE RGB(90,139,255)
#define C_PURPLE RGB(166,117,255)
#define C_GREEN RGB(71,205,145)
#define C_ORANGE RGB(255,169,93)
#define C_PINK RGB(247,103,158)
#define C_CARD RGB(34,42,66)
#define C_CARD_HI RGB(54,65,98)
#define C_NAV RGB(27,33,51)
#define C_DARK RGB(16,20,34)
#define C_BLACK RGB(0,0,0)
#define MAX_PIXEL_COUNT ((size_t)MAX_SCREEN_W * MAX_SCREEN_H)
#define PIXEL_COUNT ((size_t)SCREEN_W * SCREEN_H)

typedef struct {
    const ui_glyph_t *glyphs;
    size_t glyph_count;
    const uint8_t *data;
    int ascent;
    int line_height;
} ui_font_t;

static const ui_font_t font24 = {
    ui_font_24_glyphs, sizeof(ui_font_24_glyphs) / sizeof(ui_font_24_glyphs[0]),
    ui_font_24_data, UI_FONT_24_ASCENT, UI_FONT_24_LINE_HEIGHT
};
static const ui_font_t font32 = {
    ui_font_32_glyphs, sizeof(ui_font_32_glyphs) / sizeof(ui_font_32_glyphs[0]),
    ui_font_32_data, UI_FONT_32_ASCENT, UI_FONT_32_LINE_HEIGHT
};
extern const uint8_t ui_wallpaper_png_start[], ui_wallpaper_png_end[];
extern const uint8_t ui_app_icons_png_start[], ui_app_icons_png_end[];
static uint32_t wallpaper[1080u * 1920u];
static uint32_t app_icon_atlas[768u * 512u];
static int wallpaper_ready;
static int app_icons_ready;
static char current_clock[6] = "00:00";
static char current_date[48] = "TARİH OKUNUYOR";

enum app_id { APP_PHONE, APP_MESSAGES, APP_BROWSER, APP_SETTINGS, APP_FILES, APP_MUSIC };
static const char *const app_names[APP_COUNT] = {
    "Telefon", "Mesajlar", "Tarayıcı", "Ayarlar", "Dosyalar", "Müzik"
};
static const int resolution_widths[]={480,544,640};
static const int resolution_heights[]={854,960,1136};
static const char *const resolution_labels[]={"480 x 854","544 x 960","640 x 1136"};
static const char *const resolution_settings_labels[]={"Çözünürlük  480 x 854","Çözünürlük  544 x 960","Çözünürlük  640 x 1136"};
static const char *const resolution_about[]={"Ekran: 480 x 854","Ekran: 544 x 960","Ekran: 640 x 1136"};

static volatile uint32_t *framebuffer;
static int mouse_x;
static int mouse_y;
static int mouse_buttons;
static int mouse_packet_index;
static uint8_t mouse_packet[3];
static int home_touch_pending, home_touch_app = -1, home_touch_x, home_touch_y;
static int pointer_visible;
static int pointer_old_x;
static int pointer_old_y;
static uint32_t pointer_saved[20][16];
static uint32_t transition_from[MAX_PIXEL_COUNT];
static uint32_t transition_to[MAX_PIXEL_COUNT];
/* Render full UI frames off-screen so input redraws never expose a partial clear. */
static uint32_t render_buffer[MAX_PIXEL_COUNT];
static int current_app = -1;
static int selected_app;
static int home_page_count = 1, home_page;
static int app_pages[APP_COUNT];
static int settings_wifi = 1, settings_bluetooth, settings_brightness = 85;
static int settings_volume = 70, settings_dnd, settings_animation_mode, settings_animation_speed = 1, settings_home_animation;
static int settings_wallpaper;
static int settings_resolution=1, settings_developer_mode;
static int settings_about, selected_file = -1, text_focus;
static int phone_call_requested, music_play_requested;
static char dial_number[24];
static char message_draft[48], last_message[48];
static char browser_address[48] = "antigramic.local";
static char browser_status[48] = "Çevrimdışı başlangıç sayfası";
static const char *const file_names[] = {"home.png", "app-icons.png", "DejaVuSans.ttf", "LICENSE-DejaVu.txt"};
static int transition_origin_x = 90;
static int transition_origin_y = 547;
static uint64_t tsc_ticks_per_second = 1000000000ULL;
static uint64_t frame_start_tsc, frame_duration_tsc;

static void render_current_screen(uint32_t *target);
static void copy_to_display(const uint32_t *source);
static void erase_pointer(void);
static void redraw_current_screen(void);

static uint32_t pci_config_read(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    uint32_t address = 0x80000000U | ((uint32_t)bus << 16) |
                       ((uint32_t)device << 11) | ((uint32_t)function << 8) |
                       (offset & 0xFC);
    outl(0xCF8, address);
    return inl(0xCFC);
}

static uintptr_t find_framebuffer(void) {
    /* QEMU's PCI VGA BAR can be assigned at a different address each boot. */
    for (uint8_t device = 0; device < 32; device++) {
        uint32_t id = pci_config_read(0, device, 0, 0);
        if ((id & 0xFFFF) != 0x1234 || (id >> 16) != 0x1111) continue;
        uint32_t bar = pci_config_read(0, device, 0, 0x10);
        uintptr_t base = (uintptr_t)(bar & 0xFFFFFFF0U);
        if (base) return base;
    }
    /* ISA VGA uses the legacy Bochs VBE linear framebuffer address. */
    return 0xE0000000U;
}

static int set_vbe_resolution(unsigned choice) {
    if(choice>=sizeof(resolution_widths)/sizeof(resolution_widths[0]))return 0;
    outw(0x01CE,0);
    if(inw(0x01CF)<0xB0C4)return 0;
    outw(0x01CE,4);outw(0x01CF,0);
    outw(0x01CE,1);outw(0x01CF,(uint16_t)resolution_widths[choice]);
    outw(0x01CE,2);outw(0x01CF,(uint16_t)resolution_heights[choice]);
    outw(0x01CE,3);outw(0x01CF,32);
    outw(0x01CE,6);outw(0x01CF,(uint16_t)resolution_widths[choice]);
    outw(0x01CE,4);outw(0x01CF,0x41);
    return 1;
}

static int apply_resolution(unsigned choice) {
    if(choice>=sizeof(resolution_widths)/sizeof(resolution_widths[0]))return 0;
    int old_width=SCREEN_W,old_height=SCREEN_H;
    erase_pointer();
    if(!set_vbe_resolution(choice))return 0;
    screen_width=resolution_widths[choice];
    screen_height=resolution_heights[choice];
    mouse_x=mouse_x*SCREEN_W/old_width;
    mouse_y=mouse_y*SCREEN_H/old_height;
    if(mouse_x>SCREEN_W-12)mouse_x=SCREEN_W-12;
    if(mouse_y>SCREEN_H-20)mouse_y=SCREEN_H-20;
    pointer_visible=0;
    settings_resolution=(int)choice;
    transition_origin_x=SCREEN_W/2;
    transition_origin_y=SCREEN_H/2;
    redraw_current_screen();
    return 1;
}

static int integer_sqrt(int value) {
    int result = 0;
    int bit = 1 << 14;
    while (bit > value) bit >>= 2;
    while (bit) {
        if (value >= result + bit) {
            value -= result + bit;
            result = (result >> 1) + bit;
        } else result >>= 1;
        bit >>= 2;
    }
    return result;
}

static void output_rect(int x, int y, int w, int h, uint32_t color) {
    int x0 = x < 0 ? 0 : x;
    int y0 = y < 0 ? 0 : y;
    int x1 = x + w > SCREEN_W ? SCREEN_W : x + w;
    int y1 = y + h > SCREEN_H ? SCREEN_H : y + h;
    if (x0 >= x1 || y0 >= y1) return;
    for (int py = y0; py < y1; py++) {
        volatile uint32_t *row = framebuffer + (size_t)py * SCREEN_W;
        for (int px = x0; px < x1; px++) row[px] = color;
    }
}

static void rect(int x, int y, int w, int h, uint32_t color) {
    if (w <= 0 || h <= 0) return;
    int x0 = x * SCREEN_W / LOGICAL_W;
    int y0 = y * SCREEN_H / LOGICAL_H;
    int x1 = (x + w) * SCREEN_W / LOGICAL_W;
    int y1 = (y + h) * SCREEN_H / LOGICAL_H;
    if ((x + w) * SCREEN_W % LOGICAL_W) x1++;
    if ((y + h) * SCREEN_H % LOGICAL_H) y1++;
    output_rect(x0, y0, x1 - x0, y1 - y0, color);
}

static void rounded_rect(int x, int y, int w, int h, int radius, uint32_t color) {
    int px0 = x * SCREEN_W / LOGICAL_W, py0 = y * SCREEN_H / LOGICAL_H;
    int px1 = (x + w) * SCREEN_W / LOGICAL_W;
    int py1 = (y + h) * SCREEN_H / LOGICAL_H;
    if ((x + w) * SCREEN_W % LOGICAL_W) px1++;
    if ((y + h) * SCREEN_H % LOGICAL_H) py1++;
    int width = px1 - px0, height = py1 - py0, r = radius * SCREEN_H / LOGICAL_H;
    if (r < 1) { output_rect(px0, py0, width, height, color); return; }
    if (r * 2 > width) r = width / 2;
    if (r * 2 > height) r = height / 2;
    for (int py = 0; py < height; py++) {
        int dy = py < r ? r - py : (py >= height - r ? py - (height - r - 1) : 0);
        int inset = dy ? r - integer_sqrt(r * r - dy * dy) : 0;
        output_rect(px0 + inset, py0 + py, width - 2 * inset, 1, color);
    }
}

static uint32_t blend(uint32_t back, uint32_t front, uint8_t alpha) {
    uint32_t inverse = 255 - alpha;
    uint32_t r = (((back >> 16) & 255) * inverse + ((front >> 16) & 255) * alpha) / 255;
    uint32_t g = (((back >> 8) & 255) * inverse + ((front >> 8) & 255) * alpha) / 255;
    uint32_t b = ((back & 255) * inverse + (front & 255) * alpha) / 255;
    return RGB(r, g, b);
}

static void draw_icon_image(int app,int x,int y,int size) {
    if(!app_icons_ready)return;
    int px=x*SCREEN_W/LOGICAL_W,py=y*SCREEN_H/LOGICAL_H;
    int pw=size*SCREEN_W/LOGICAL_W,ph=size*SCREEN_H/LOGICAL_H;
    int src_x=(app%3)*256,src_y=(app/3)*256;
    for(int dy=0;dy<ph;dy++)for(int dx=0;dx<pw;dx++){
        unsigned sx=(unsigned)src_x+(unsigned)dx*256/(unsigned)pw;
        unsigned sy=(unsigned)src_y+(unsigned)dy*256/(unsigned)ph;
        uint32_t pixel=app_icon_atlas[(size_t)sy*768+sx];
        uint8_t alpha=(uint8_t)(pixel>>24);
        if(alpha){size_t at=(size_t)(py+dy)*SCREEN_W+px+dx;framebuffer[at]=blend(framebuffer[at],pixel,alpha);}
    }
}

static const ui_glyph_t *find_glyph(const ui_font_t *font, uint32_t codepoint) {
    for (size_t i = 0; i < font->glyph_count; i++)
        if (font->glyphs[i].codepoint == codepoint) return &font->glyphs[i];
    return NULL;
}

static uint32_t next_codepoint(const char **value) {
    const uint8_t *p = (const uint8_t *)*value;
    uint32_t cp;
    if (p[0] < 0x80) { cp = p[0]; *value += 1; }
    else if ((p[0] & 0xE0) == 0xC0 && p[1]) {
        cp = ((uint32_t)(p[0] & 0x1F) << 6) | (p[1] & 0x3F); *value += 2;
    } else if ((p[0] & 0xF0) == 0xE0 && p[1] && p[2]) {
        cp = ((uint32_t)(p[0] & 0x0F) << 12) | ((uint32_t)(p[1] & 0x3F) << 6) | (p[2] & 0x3F); *value += 3;
    } else { cp = '?'; *value += 1; }
    return cp;
}

static int measure_text(const ui_font_t *font, const char *value, int scale) {
    int width = 0;
    while (*value) {
        const ui_glyph_t *glyph = find_glyph(font, next_codepoint(&value));
        if (!glyph) glyph = find_glyph(font, '?');
        if (glyph) width += glyph->advance * scale;
    }
    return width;
}

static void draw_glyph(int x, int baseline, const ui_font_t *font,
                       const ui_glyph_t *glyph, int scale, uint32_t color) {
    int origin_x = x + glyph->left * scale;
    int origin_y = baseline + glyph->top * scale;
    int out_w=(glyph->width*scale*SCREEN_W+LOGICAL_W/2)/LOGICAL_W;
    int out_h=(glyph->height*scale*SCREEN_H+LOGICAL_H/2)/LOGICAL_H;
    if(out_w<1)out_w=1;
    if(out_h<1)out_h=1;
    int px0=origin_x*SCREEN_W/LOGICAL_W,py0=origin_y*SCREEN_H/LOGICAL_H;
    for(int dy=0;dy<out_h;dy++)for(int dx=0;dx<out_w;dx++){
        int gx0=dx*glyph->width/out_w,gx1=((dx+1)*glyph->width+out_w-1)/out_w;
        int gy0=dy*glyph->height/out_h,gy1=((dy+1)*glyph->height+out_h-1)/out_h;
        if(gx1<=gx0)gx1=gx0+1;
        if(gy1<=gy0)gy1=gy0+1;
        unsigned coverage=0,samples=0;
        for(int gy=gy0;gy<gy1&&gy<glyph->height;gy++)for(int gx=gx0;gx<gx1&&gx<glyph->width;gx++){
            size_t i=(size_t)gy*glyph->width+gx;
            coverage+=(font->data[glyph->offset+i/4]>>(6-((i%4)*2)))&3;
            samples++;
        }
        if(!coverage||!samples)continue;
        int tx=px0+dx,ty=py0+dy;
        if((unsigned)tx<(unsigned)SCREEN_W&&(unsigned)ty<(unsigned)SCREEN_H){
            size_t at=(size_t)ty*SCREEN_W+tx;
            framebuffer[at]=blend(framebuffer[at],color,(uint8_t)(coverage*85/samples));
        }
    }
}

static void draw_text(int x, int y, const char *value, const ui_font_t *font,
                      int scale, uint32_t color) {
    int baseline = y + font->ascent * scale;
    while (*value) {
        const ui_glyph_t *glyph = find_glyph(font, next_codepoint(&value));
        if (!glyph) glyph = find_glyph(font, '?');
        if (!glyph) continue;
        draw_glyph(x, baseline, font, glyph, scale, color);
        x += glyph->advance * scale;
    }
}

static void center_text(int center_x, int y, const char *value, const ui_font_t *font,
                        int scale, uint32_t color) {
    draw_text(center_x - measure_text(font, value, scale) / 2, y, value, font, scale, color);
}

static void percentage_string(unsigned value,char output[5]) {
    if(value>100)value=100;
    unsigned at=0;
    if(value>=100)output[at++]='1';
    else if(value>=10)output[at++]=(char)('0'+value/10);
    output[at++]=(char)('0'+value%10);
    output[at++]='%';
    output[at]=0;
}

static void draw_background(void) {
    if (wallpaper_ready) {
        for (int y = 0; y < SCREEN_H; y++) {
            unsigned sy0 = (unsigned)y * LOGICAL_H / SCREEN_H;
            unsigned sy1 = (unsigned)(y + 1) * LOGICAL_H / SCREEN_H;
            for (int x = 0; x < SCREEN_W; x++) {
                unsigned sx0 = (unsigned)x * LOGICAL_W / SCREEN_W;
                unsigned sx1 = (unsigned)(x + 1) * LOGICAL_W / SCREEN_W;
                unsigned rr=0,gg=0,bb=0,count=0;
                for(unsigned sy=sy0;sy<sy1;sy++) for(unsigned sx=sx0;sx<sx1;sx++) {
                    uint32_t c=wallpaper[(size_t)sy*LOGICAL_W+sx]; rr+=(c>>16)&255;gg+=(c>>8)&255;bb+=c&255;count++;
                }
                uint32_t color=RGB(rr/count,gg/count,bb/count);
                if(settings_wallpaper==1)color=blend(color,RGB(30,178,189),68);
                else if(settings_wallpaper==2)color=blend(color,RGB(211,83,102),72);
                framebuffer[(size_t)y*SCREEN_W+x]=color;
            }
        }
        return;
    }
    for (int y = 0; y < SCREEN_H; y++) {
        uint32_t t = (uint32_t)y * 255 / (SCREEN_H - 1);
        uint32_t r = (((C_BG >> 16) & 255) * (255 - t) + ((C_BG2 >> 16) & 255) * t) / 255;
        uint32_t g = (((C_BG >> 8) & 255) * (255 - t) + ((C_BG2 >> 8) & 255) * t) / 255;
        uint32_t b = ((C_BG & 255) * (255 - t) + (C_BG2 & 255) * t) / 255;
        output_rect(0, y, SCREEN_W, 1, RGB(r, g, b));
    }
    rounded_rect(80, 370, 920, 920, 460, RGB(36,49,88));
    rounded_rect(230, 520, 620, 620, 310, RGB(43,42,93));
}

static void draw_status_bar(void) {
    rect(0, 0, LOGICAL_W, 96, C_DARK);
    draw_text(34, 16, current_clock, &font24, 2, C_WHITE);
    if(settings_dnd){rounded_rect(190,32,20,20,10,C_PURPLE);rounded_rect(198,28,18,18,9,C_DARK);}
    rect(848, 44, 8, 12, C_WHITE); rect(864, 36, 8, 20, C_WHITE);
    rect(880, 28, 8, 28, C_WHITE); rect(896, 20, 8, 36, C_WHITE);
    rounded_rect(942, 24, 76, 40, 10, C_WHITE);
    rect(1020, 34, 8, 20, C_WHITE);
    rounded_rect(950, 32, 52, 24, 5, C_GREEN);
    if(settings_developer_mode){
        draw_text(700,30,"DEV",&font24,1,C_CYAN);
        draw_text(748,30,resolution_labels[settings_resolution],&font24,1,C_WHITE);
    }
}

static void draw_app_icon(int app, int cx, int y, int size, int selected) {
    int x = cx - size / 2;
    if(selected)rounded_rect(x-7,y-7,size+14,size+14,42,C_WHITE);
    draw_icon_image(app,x,y,size);
}

static void apply_display_brightness(void) {
    unsigned level=(unsigned)settings_brightness;
    for(size_t i=0;i<PIXEL_COUNT;i++){
        uint32_t c=framebuffer[i];
        framebuffer[i]=RGB(((c>>16)&255)*level/100,((c>>8)&255)*level/100,(c&255)*level/100);
    }
}

static void draw_home(void) {
    draw_background();
    draw_status_bar();
    static const int centers[3] = { 180, 540, 900 };
    int visible=0;
    for (int i = 0; i < APP_COUNT; i++) if(app_pages[i]==home_page) {
        int row = visible / 3, col = visible % 3;
        int icon_y = row ? 1320 : 1010;
        int label_y = row ? 1510 : 1200;
        draw_app_icon(i, centers[col], icon_y, 170, i == selected_app);
        center_text(centers[col], label_y, app_names[i], &font24, 2, C_WHITE);
        visible++;
    }
    if(home_page==0){
        draw_text(64,390,current_date,&font24,2,C_MUTED);
        center_text(540,510,current_clock,&font32,6,C_WHITE);
        center_text(540,755,"SİSTEM SAATİ",&font24,2,C_MUTED);
    }
    rounded_rect(36, 1620, 1008, 148, 70, C_NAV);
    const int dock_apps[4] = { APP_PHONE, APP_MESSAGES, APP_BROWSER, APP_SETTINGS };
    const int dock_x[4] = { 180, 420, 660, 900 };
    for (int i = 0; i < 4; i++) draw_app_icon(dock_apps[i], dock_x[i], 1645, 108, 0);
    for(int i=0;i<home_page_count;i++)rounded_rect(472+i*36,1800,18,18,9,i==home_page?C_WHITE:C_MUTED);
    rounded_rect(880,1780,112,64,24,home_page_count<HOME_MAX_PAGES?C_CARD_HI:C_NAV);
    center_text(936,1784,home_page_count<HOME_MAX_PAGES?"+":"MAX",&font24,1,C_WHITE);
    rounded_rect(450, 1878, 180, 12, 6, C_WHITE);
}

static int home_grid_app_at(int x,int y) {
    if(y<1000||y>1610)return -1;
    int row=y<1320?0:1,col=x/360;
    if(col<0||col>2)return -1;
    int slot=row*3+col,visible=0;
    for(int app=0;app<APP_COUNT;app++)if(app_pages[app]==home_page){
        if(visible==slot)return app;
        visible++;
    }
    return -1;
}

static void draw_back_button(void) {
    for (int i = 0; i < 7; i++) {
        rect(34 + i*7, 136 + i*7, 10, 10, C_WHITE);
        rect(34 + i*7, 220 - i*7, 10, 10, C_WHITE);
    }
    rounded_rect(34, 174, 88, 16, 8, C_WHITE);
}

static void draw_app_screen(void) {
    draw_background();
    draw_status_bar();
    rect(0, 96, LOGICAL_W, 120, C_DARK);
    draw_back_button();
    draw_text(150, 124, app_names[current_app], &font32, 2, C_WHITE);
    rounded_rect(32, 250, 1016, 1420, 44, C_CARD);

    if (current_app == APP_SETTINGS) {
        if(settings_about){
            draw_text(76,300,"CİHAZ BİLGİSİ",&font24,2,C_CYAN);
            draw_text(76,440,"AntigramicOS",&font32,2,C_WHITE);
            draw_text(76,550,"x86_64 deneysel sistem",&font24,2,C_MUTED);
            draw_text(76,670,resolution_about[settings_resolution],&font24,2,C_MUTED);
            draw_text(76,770,"Arayüz: 1080 x 1920",&font24,2,C_MUTED);
            draw_text(76,900,"Bellek: 128 MiB",&font24,2,C_MUTED);
            draw_text(76,1010,settings_animation_mode==0?"Hareket hedefi: 120 FPS":
                settings_animation_mode==1?"Hareket hedefi: 60 FPS":"Hareket: Azaltılmış",&font24,2,C_MUTED);
            draw_text(76,1120,settings_wifi?"Wi-Fi anahtarı: Açık":"Wi-Fi anahtarı: Kapalı",&font24,1,C_MUTED);
            rounded_rect(76,1340,928,112,36,C_CARD_HI);
            center_text(540,1370,"AYARLARA DÖN",&font24,2,C_WHITE);
        } else {
            draw_text(76,286,"AYARLAR",&font24,2,C_CYAN);
            const char *wifi=settings_wifi?"Wi-Fi                         Açık":"Wi-Fi                         Kapalı";
            const char *bluetooth=settings_bluetooth?"Bluetooth                   Açık":"Bluetooth                   Kapalı";
            const char *brightness=settings_brightness>50?"Parlaklık":"Parlaklık";
            const char *volume="Ses düzeyi";
            const char *dnd=settings_dnd?"Rahatsız etme             Açık":"Rahatsız etme             Kapalı";
            const char *motion=settings_animation_mode==0?"Hareket hızı              120 FPS":
                settings_animation_mode==1?"Hareket hızı               60 FPS":"Hareket hızı              Azaltılmış";
            const char *speed=settings_animation_speed==0?"Animasyon süresi        Yavaş":
                settings_animation_speed==1?"Animasyon süresi        Normal":"Animasyon süresi        Hızlı";
            const char *home_motion=settings_home_animation?"Ana ekran animasyonu     Açık":"Ana ekran animasyonu     Kapalı";
            const char *wallpaper_name=settings_wallpaper==0?"Duvar kağıdı        Gece":settings_wallpaper==1?"Duvar kağıdı        Okyanus":"Duvar kağıdı        Gün batımı";
            const char *resolution=resolution_settings_labels[settings_resolution];
            const char *developer=settings_developer_mode?"Geliştirici modu      Açık":"Geliştirici modu      Kapalı";
            const char *rows[]={wifi,bluetooth,brightness,volume,dnd,motion,speed,home_motion,wallpaper_name,resolution,developer,"Cihaz hakkında"};
            for(int i=0;i<12;i++){
                int y=350+i*105;
                rounded_rect(76,y,928,94,22,C_CARD_HI);
                draw_text(104,y+6,rows[i],&font24,2,C_WHITE);
                if(i==2||i==3){
                    int value=i==2?settings_brightness:settings_volume;
                    char percentage[5];
                    percentage_string((unsigned)value,percentage);
                    center_text(910,y+18,percentage,&font24,2,C_WHITE);
                    rounded_rect(104,y+62,872,14,7,C_NAV);
                    rounded_rect(104,y+62,value*872/100,14,7,i==2?C_CYAN:C_PURPLE);
                    int knob=104+value*872/100;
                    rounded_rect(knob-9,y+58,18,28,9,C_WHITE);
                }
            }
        }
    } else if (current_app == APP_FILES) {
        draw_text(76,300,"PAKETTEKİ DOSYALAR",&font24,2,C_CYAN);
        for(int i=0;i<4;i++){
            int y=390+i*190;
            rounded_rect(76,y,928,142,30,selected_file==i?C_CARD_HI:C_CARD);
            draw_icon_image(APP_FILES,96,y+28,86);
            draw_text(220,y+30,file_names[i],&font24,2,C_WHITE);
            draw_text(220,y+82,i<2?"PNG GÖRSELİ":"FONT DOSYASI",&font24,1,C_MUTED);
        }
        if(selected_file>=0){
            draw_text(76,1190,"SEÇİLEN DOSYA",&font24,2,C_CYAN);
            draw_text(76,1270,file_names[selected_file],&font24,2,C_WHITE);
            static const char *const details[]={"PNG görseli · 1080 x 1920","PNG simge sayfası · 768 x 512",
                "Türkçe yazı tipi","Yazı tipi lisansı"};
            draw_text(76,1360,details[selected_file],&font24,1,C_MUTED);
        }
    } else if (current_app == APP_BROWSER) {
        rounded_rect(62,290,956,112,42,text_focus?C_CARD_HI:C_NAV);
        draw_text(100,320,browser_address,&font24,2,text_focus?C_WHITE:C_MUTED);
        center_text(540,600,"ANTİGRAMİC TARAYICI",&font24,2,C_CYAN);
        center_text(540,740,browser_status,&font24,1,C_WHITE);
        rounded_rect(300,1330,480,112,48,C_BLUE);
        center_text(540,1358,"ADRESİ AÇ",&font24,2,C_WHITE);
    } else if (current_app == APP_PHONE) {
        rounded_rect(82,292,916,126,34,C_NAV);
        center_text(540,326,dial_number[0]?dial_number:"Numara girin",&font32,2,C_WHITE);
        static const char *const keys[]={"1","2","3","4","5","6","7","8","9","*","0","#"};
        for(int i=0;i<12;i++){
            int row=i/3,col=i%3,cx=270+col*270,y=460+row*198;
            rounded_rect(cx-94,y,188,154,56,C_CARD_HI);
            center_text(cx,y+34,keys[i],&font32,3,C_WHITE);
        }
        rounded_rect(360,1285,360,120,56,C_GREEN);
        center_text(540,1315,"ARA",&font24,2,C_WHITE);
        center_text(540,1460,phone_call_requested?"SIM / MODEM BULUNAMADI":"Arama için numara girin",&font24,1,C_MUTED);
    } else if (current_app == APP_MESSAGES) {
        draw_text(76,300,"YEREL SOHBET",&font24,2,C_CYAN);
        rounded_rect(76,410,928,670,32,C_NAV);
        if(last_message[0]){
            rounded_rect(150,470,800,150,32,C_CARD_HI);
            draw_text(185,505,last_message,&font24,1,C_WHITE);
            draw_text(185,565,"Gönderilen mesaj",&font24,1,C_MUTED);
        }else center_text(540,680,"Henüz mesaj yok",&font24,2,C_MUTED);
        rounded_rect(76,1280,928,116,30,text_focus?C_CARD_HI:C_NAV);
        draw_text(104,1310,message_draft[0]?message_draft:"Mesaj yazmak için tıkla",&font24,1,text_focus?C_WHITE:C_MUTED);
        rounded_rect(760,1440,244,112,40,C_BLUE);
        center_text(882,1468,"GÖNDER",&font24,1,C_WHITE);
        draw_text(88,1600,"Klavye ile yaz, Enter ile gönder",&font24,1,C_MUTED);
    } else {
        center_text(540,370,"MÜZİK ÇALAR",&font24,2,C_CYAN);
        draw_icon_image(APP_MUSIC,380,520,320);
        center_text(540,900,"ÖRNEK ÇALMA DURUMU",&font24,2,C_WHITE);
        center_text(540,1000,music_play_requested?"Çalma isteği etkin":"Beklemede",&font24,1,C_MUTED);
        rounded_rect(318,1230,444,142,66,music_play_requested?C_PINK:C_CARD_HI);
        center_text(540,1271,music_play_requested?"DURAKLAT":"ÇAL",&font24,2,C_WHITE);
        center_text(540,1460,"Ses aygıtı sürücüsü bekleniyor",&font24,1,C_MUTED);
    }
    rounded_rect(450, 1878, 180, 12, 6, C_WHITE);
}

static void draw_pointer(void) {
    static const uint16_t shape[20] = {
        0x8000,0xC000,0xE000,0xF000,0xF800,0xFC00,0xFE00,0xFF00,
        0xFF80,0xFFC0,0xFFE0,0xFFF0,0xFFF8,0xFFC0,0xCFE0,0x87F0,
        0x03F0,0x01F8,0x00F8,0x0070
    };
    for (int y = 0; y < 20; y++) for (int x = 0; x < 16; x++) {
        int px = mouse_x + x, py = mouse_y + y;
        pointer_saved[y][x] = (unsigned)px < (unsigned)SCREEN_W && (unsigned)py < (unsigned)SCREEN_H
            ? framebuffer[(size_t)py * SCREEN_W + px] : C_BLACK;
        int inside = !!(shape[y] & (0x8000 >> x)), edge = 0;
        if (!inside) for (int dy=-1;dy<=1;dy++) for(int dx=-1;dx<=1;dx++) {
            int xx=x+dx,yy=y+dy;
            if(xx>=0&&xx<16&&yy>=0&&yy<20&&(shape[yy]&(0x8000>>xx)))edge=1;
        }
        if (inside || edge) output_rect(px, py, 1, 1, inside ? C_WHITE : C_DARK);
    }
    pointer_old_x = mouse_x;
    pointer_old_y = mouse_y;
    pointer_visible = 1;
}

static void erase_pointer(void) {
    if (!pointer_visible) return;
    for (int y = 0; y < 20; y++) for (int x = 0; x < 16; x++) {
        int px = pointer_old_x + x, py = pointer_old_y + y;
        if ((unsigned)px < (unsigned)SCREEN_W && (unsigned)py < (unsigned)SCREEN_H)
            framebuffer[(size_t)py * SCREEN_W + px] = pointer_saved[y][x];
    }
    pointer_visible = 0;
}

static unsigned animation_fps(void) {
    return settings_animation_mode==1?60:120;
}

/* Calibrated TSC pacing gives both directions the same 120/60 Hz cadence. */
static uint64_t read_tsc(void) {
    uint32_t low,high;
    asm volatile("lfence; rdtsc":"=a"(low),"=d"(high)::"memory");
    return ((uint64_t)high<<32)|low;
}

static void calibrate_tsc(void) {
    rtc_time_t first,current;
    if(!rtc_read(&first))return;
    uint64_t start=read_tsc();
    for(uint32_t i=0;i<500000;i++){
        if(rtc_read(&current)&&current.second!=first.second){
            uint64_t elapsed=read_tsc()-start;
            if(elapsed>1000000)tsc_ticks_per_second=elapsed;
            return;
        }
        asm volatile("pause");
    }
}

static void frame_timer_begin(void) {
    frame_start_tsc=read_tsc();
    frame_duration_tsc=tsc_ticks_per_second/animation_fps();
}

static void frame_timer_wait(void) {
    while(read_tsc()-frame_start_tsc<frame_duration_tsc)asm volatile("pause");
}

static unsigned animation_duration_ms(void) {
    static const unsigned durations[]={350,200,120};
    return durations[settings_animation_speed];
}

static unsigned animation_frame_count(void) {
    unsigned frames=animation_fps()*animation_duration_ms()/1000;
    return frames?frames:1;
}

static uint32_t eased_progress(unsigned frame,unsigned frames) {
    uint32_t t=(uint32_t)(((uint64_t)(frame+1)*65536)/frames);
    uint32_t t2=(uint32_t)(((uint64_t)t*t)>>16);
    return (uint32_t)(((uint64_t)t2*(3u*65536u-2u*t))>>16);
}

static void paint_slide_frame(unsigned offset,int forward) {
    if(offset>(unsigned)SCREEN_W)offset=(unsigned)SCREEN_W;
    /* Keep the status and gesture bars pinned while the launcher pages move. */
    for(int y=SYSTEM_TOP_END;y<SYSTEM_BOTTOM_START;y++){
        volatile uint32_t *out=framebuffer+(size_t)y*SCREEN_W;
        const uint32_t *old=transition_from+(size_t)y*SCREEN_W;
        const uint32_t *next=transition_to+(size_t)y*SCREEN_W;
        for(int x=0;x<SCREEN_W;x++){
            int source=forward?x+(int)offset:x-(int)offset;
            out[x]=source>=0&&source<SCREEN_W?old[source]:
                next[source<0?source+SCREEN_W:source-SCREEN_W];
        }
    }
}

static void restore_fixed_system_bars(void) {
    for(int y=0;y<SYSTEM_TOP_END;y++){
        size_t row=(size_t)y*SCREEN_W;
        for(int x=0;x<SCREEN_W;x++)framebuffer[row+x]=transition_from[row+x];
    }
    for(int y=SYSTEM_BOTTOM_START;y<SCREEN_H;y++){
        size_t row=(size_t)y*SCREEN_W;
        for(int x=0;x<SCREEN_W;x++)framebuffer[row+x]=transition_from[row+x];
    }
}

static void switch_home_page(int target,int moving_app) {
    if(target<0||target>=home_page_count||target==home_page)return;
    erase_pointer();
    for(size_t i=0;i<PIXEL_COUNT;i++)transition_from[i]=framebuffer[i];
    int previous=home_page;
    home_page=target;
    if(moving_app>=0&&moving_app<APP_COUNT)app_pages[moving_app]=target;
    render_current_screen(transition_to);
    if(settings_animation_mode==2){
        copy_to_display(transition_to);
        draw_pointer();
        return;
    }
    unsigned frames=animation_frame_count();
    int forward=target>previous;
    for(unsigned frame=0;frame<frames;frame++){
        unsigned offset=(unsigned)(((uint64_t)SCREEN_W*eased_progress(frame,frames))>>16);
        frame_timer_begin();
        paint_slide_frame(offset,forward);
        frame_timer_wait();
    }
    draw_pointer();
}

static void paint_zoomed_target(int x,int y,int width,int height) {
    uint32_t xstep=(uint32_t)(((uint64_t)SCREEN_W<<16)/(unsigned)width);
    uint32_t ystep=(uint32_t)(((uint64_t)SCREEN_H<<16)/(unsigned)height);
    uint32_t sy=0;
    for(int dy=0;dy<height;dy++,sy+=ystep){
        int py=y+dy;
        if((unsigned)py>=(unsigned)SCREEN_H)continue;
        uint32_t sx=0;
        for(int dx=0;dx<width;dx++,sx+=xstep){
            int px=x+dx;
            int radius=18,edge_x=dx<radius?radius-dx:dx>=width-radius?dx-(width-radius-1):0;
            int edge_y=dy<radius?radius-dy:dy>=height-radius?dy-(height-radius-1):0;
            int rounded=edge_x&&edge_y&&edge_x*edge_x+edge_y*edge_y>radius*radius;
            if((unsigned)px<(unsigned)SCREEN_W&&!rounded)
                framebuffer[(size_t)py*SCREEN_W+px]=transition_to[(size_t)(sy>>16)*SCREEN_W+(sx>>16)];
        }
    }
}

/* One icon-origin zoom curve is shared by launch and return transitions. */
static void render_current_screen(uint32_t *target) {
    volatile uint32_t *physical=framebuffer;
    framebuffer=target;
    if(current_app<0)draw_home();else draw_app_screen();
    apply_display_brightness();
    framebuffer=physical;
}

static void copy_to_display(const uint32_t *source) {
    volatile uint32_t *display=framebuffer;
    for(size_t i=0;i<PIXEL_COUNT;i++)display[i]=source[i];
}

static void animate_transition(int to_home) {
    erase_pointer();
    for(size_t i=0;i<PIXEL_COUNT;i++)transition_from[i]=framebuffer[i];
    render_current_screen(transition_to);
    if((to_home&&!settings_home_animation)||settings_animation_mode==2){
        copy_to_display(transition_to);
        draw_pointer();
        return;
    }
    for(size_t i=0;i<PIXEL_COUNT;i++)framebuffer[i]=transition_from[i];

    unsigned frames=animation_frame_count();
    for(unsigned frame=0;frame<frames;frame++){
        uint32_t eased=eased_progress(frame,frames);
        int width=76+(int)(((uint64_t)(SCREEN_W-76)*eased)>>16);
        int height=76+(int)(((uint64_t)(SCREEN_H-76)*eased)>>16);
        int center_x=transition_origin_x+(int)(((int64_t)(SCREEN_W/2-transition_origin_x)*eased)/65536);
        int center_y=transition_origin_y+(int)(((int64_t)(SCREEN_H/2-transition_origin_y)*eased)/65536);
        int x=center_x-width/2,y=center_y-height/2;
        frame_timer_begin();
        paint_zoomed_target(x,y,width,height);
        restore_fixed_system_bars();
        frame_timer_wait();
    }
    draw_pointer();
}

static void set_grid_transition_origin(int app) {
    static const int centers[3]={180,540,900};
    int slot=0;
    for(int i=0;i<app;i++)if(app_pages[i]==home_page)slot++;
    int row=slot/3,col=slot%3,icon_y=row?1320:1010;
    transition_origin_x=centers[col]*SCREEN_W/LOGICAL_W;
    transition_origin_y=(icon_y+85)*SCREEN_H/LOGICAL_H;
}

static void close_app(void) {
    current_app=-1;
    animate_transition(1);
}

static void open_app(int app) {
    if (app < 0 || app >= APP_COUNT) return;
    current_app = app;
    text_focus=(app==APP_MESSAGES||app==APP_BROWSER);
    animate_transition(0);
}

static void redraw_current_screen(void) {
    erase_pointer();
    render_current_screen(render_buffer);
    copy_to_display(render_buffer);
    draw_pointer();
}

static void clear_text(char *text,size_t capacity) {
    for(size_t i=0;i<capacity;i++)text[i]=0;
}

static void copy_text(char *to,size_t capacity,const char *from) {
    size_t i=0;
    while(i+1<capacity&&from[i]){to[i]=from[i];i++;}
    to[i]=0;
}

static void submit_message(void) {
    if(!message_draft[0])return;
    copy_text(last_message,sizeof(last_message),message_draft);
    clear_text(message_draft,sizeof(message_draft));
    text_focus=0;
    redraw_current_screen();
}

static void handle_click(int x, int y) {
    int lx = x * LOGICAL_W / SCREEN_W, ly = y * LOGICAL_H / SCREEN_H;
    if (current_app >= 0) {
        if (ly < 216 && lx < 130) { close_app(); return; }
        if(current_app==APP_SETTINGS){
            if(settings_about){settings_about=0;redraw_current_screen();return;}
            if(ly>=350&&ly<350+12*105){
                int row=(ly-350)/105;
                if(row==0)settings_wifi=!settings_wifi;
                else if(row==1)settings_bluetooth=!settings_bluetooth;
                else if(row==2){
                    if(ly>=350+2*105+50&&ly<350+2*105+90)settings_brightness=lx*100/LOGICAL_W;
                    else settings_brightness=settings_brightness>=80?40:settings_brightness+20;
                } else if(row==3){
                    if(ly>=350+3*105+50&&ly<350+3*105+90)settings_volume=lx*100/LOGICAL_W;
                    else settings_volume=settings_volume>=90?30:settings_volume+20;
                } else if(row==4)settings_dnd=!settings_dnd;
                else if(row==5)settings_animation_mode=(settings_animation_mode+1)%3;
                else if(row==6)settings_animation_speed=(settings_animation_speed+1)%3;
                else if(row==7)settings_home_animation=!settings_home_animation;
                else if(row==8)settings_wallpaper=(settings_wallpaper+1)%3;
                else if(row==9)apply_resolution((unsigned)(settings_resolution+1)%3);
                else if(row==10)settings_developer_mode=!settings_developer_mode;
                else if(row==11)settings_about=1;
            }
            redraw_current_screen();
        } else if(current_app==APP_PHONE){
            if(ly>=460&&ly<1210){
                int row=(ly-460)/198,col=lx/360,index=row*3+col;
                if(row<4&&col<3){
                    size_t n=0;while(dial_number[n]&&n+1<sizeof(dial_number))n++;
                    if(n+1<sizeof(dial_number)){dial_number[n]=(index==9?'*':index==10?'0':index==11?'#':(char)('1'+index));dial_number[n+1]=0;}
                    redraw_current_screen();
                }
            } else if(ly>=1285&&ly<1410){phone_call_requested=1;redraw_current_screen();}
            else if(ly>=292&&ly<418&&dial_number[0]){size_t n=0;while(dial_number[n])n++;if(n)dial_number[n-1]=0;redraw_current_screen();}
        } else if(current_app==APP_MESSAGES){
            if(ly>=1260&&ly<1405){text_focus=1;redraw_current_screen();}
            else if(ly>=1430&&ly<1570)submit_message();
        } else if(current_app==APP_BROWSER){
            if(ly>=280&&ly<420){text_focus=1;copy_text(browser_status,sizeof(browser_status),"Adres düzenleniyor");redraw_current_screen();}
            else if(ly>=1320&&ly<1460){text_focus=0;copy_text(browser_status,sizeof(browser_status),"Ağ bağlantısı yok");redraw_current_screen();}
        } else if(current_app==APP_FILES){
            if(ly>=390&&ly<390+4*190){selected_file=(ly-390)/190;redraw_current_screen();}
        } else if(current_app==APP_MUSIC){
            if(ly>=1210&&ly<1390){music_play_requested=!music_play_requested;redraw_current_screen();}
        }
        return;
    }
    if(ly>=1780&&ly<1850){
        if(lx>=850&&lx<1020&&home_page_count<HOME_MAX_PAGES){
            int target=home_page_count++;
            switch_home_page(target,-1);
        }else if(lx>=450&&lx<700){
            int selected=(lx-463)/36;
            if(selected>=0&&selected<home_page_count)switch_home_page(selected,-1);
        }
        redraw_current_screen();
    } else {
        int app=home_grid_app_at(lx,ly);
        if(app>=0){
            selected_app = app;
            set_grid_transition_origin(selected_app);
            open_app(selected_app);
        }
    }
    if (current_app<0&&ly >= 1610 && ly < 1770) {
        static const int dock_apps[4] = { APP_PHONE, APP_MESSAGES, APP_BROWSER, APP_SETTINGS };
        int slot = lx / 270;
        static const int dock_x[4]={180,420,660,900};
        if (slot >= 0 && slot < 4) {
            selected_app=dock_apps[slot];
            transition_origin_x=dock_x[slot]*SCREEN_W/LOGICAL_W;
            transition_origin_y=(1645+54)*SCREEN_H/LOGICAL_H;
            open_app(dock_apps[slot]);
        }
    }
}

static void keyboard_byte(uint8_t code) {
    if (code & 0x80) return;
    if (code == 0x01 || (code == 0x23 && current_app>=0 && !text_focus)) {
        if (current_app >= 0) close_app();
    } else if (current_app < 0 && code >= 0x02 && code <= 0x07) {
        selected_app=code-0x02;
        home_page=app_pages[selected_app];
        set_grid_transition_origin(selected_app);
        open_app(selected_app);
    } else if (code == 0x4D || code == 0x4B) {
        if(current_app<0){
            int page=home_page+(code==0x4D?1:-1);
            if(page>=0&&page<home_page_count)switch_home_page(page,-1);
        }else{
            selected_app = (selected_app + (code == 0x4D ? 1 : APP_COUNT - 1)) % APP_COUNT;
            redraw_current_screen();
        }
    } else if (code == 0x1C && current_app < 0) {
        home_page=app_pages[selected_app];
        set_grid_transition_origin(selected_app);
        open_app(selected_app);
    }
    else if(current_app==APP_MESSAGES||current_app==APP_BROWSER||current_app==APP_PHONE){
        if(code==0x1C){
            if(current_app==APP_MESSAGES)submit_message();
            else if(current_app==APP_BROWSER){text_focus=0;copy_text(browser_status,sizeof(browser_status),"Ağ bağlantısı yok");redraw_current_screen();}
            else {phone_call_requested=1;redraw_current_screen();}
            return;
        }
        if(current_app==APP_PHONE&&code==0x0E){
            size_t n=0;while(dial_number[n])n++;if(n)dial_number[n-1]=0;redraw_current_screen();return;
        }
        if(current_app==APP_PHONE&&code>=0x02&&code<=0x0B){
            size_t n=0;while(dial_number[n]&&n+1<sizeof(dial_number))n++;
            if(n+1<sizeof(dial_number)){dial_number[n]=code==0x0B?'0':(char)('1'+code-0x02);dial_number[n+1]=0;}
            redraw_current_screen();return;
        }
        if((current_app==APP_MESSAGES&&text_focus)||(current_app==APP_BROWSER&&text_focus)){
            char *buffer=current_app==APP_MESSAGES?message_draft:browser_address;
            size_t capacity=current_app==APP_MESSAGES?sizeof(message_draft):sizeof(browser_address);
            if(code==0x0E){size_t n=0;while(buffer[n])n++;if(n)buffer[n-1]=0;}
            else {
                static const char keys[]="1234567890qwertyuiopasdfghjkl;zxcvbnm,./- =[]";
                static const uint8_t scans[]={0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,
                    0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,
                    0x1E,0x1F,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,
                    0x2C,0x2D,0x2E,0x2F,0x30,0x31,0x32,0x33,0x34,0x35,0x0C,0x39,0x0D,0x1A,0x1B};
                char ch=0;
                for(unsigned i=0;i<sizeof(scans)/sizeof(scans[0]);i++)if(scans[i]==code){ch=keys[i];break;}
                size_t n=0;while(buffer[n]&&n+1<capacity)n++;
                if(ch&&n+1<capacity){buffer[n]=ch;buffer[n+1]=0;}
            }
            redraw_current_screen();
        }
    }
}

static void process_mouse_packet(void) {
    if (mouse_packet[0] & 0xC0) return;
    erase_pointer();
    mouse_x += (int8_t)mouse_packet[1];
    mouse_y -= (int8_t)mouse_packet[2];
    if (mouse_x < 0) mouse_x = 0;
    if (mouse_x > SCREEN_W - 12) mouse_x = SCREEN_W - 12;
    if (mouse_y < 0) mouse_y = 0;
    if (mouse_y > SCREEN_H - 20) mouse_y = SCREEN_H - 20;
    int buttons = mouse_packet[0] & 7;
    int was_pressed=mouse_buttons&1,is_pressed=buttons&1;
    if(is_pressed&&!was_pressed){
        if(current_app<0){
            home_touch_pending=1;
            home_touch_x=mouse_x;
            home_touch_y=mouse_y;
            home_touch_app=home_grid_app_at(mouse_x*LOGICAL_W/SCREEN_W,mouse_y*LOGICAL_H/SCREEN_H);
        }else handle_click(mouse_x,mouse_y);
    }else if(!is_pressed&&was_pressed&&home_touch_pending){
        int delta=mouse_x-home_touch_x;
        if(delta>SCREEN_W/8||delta<-(SCREEN_W/8)){
            int target=home_page+(delta<0?1:-1);
            if(target>=home_page_count&&delta<0&&home_page_count<HOME_MAX_PAGES)
                target=home_page_count++;
            if(target>=0&&target<home_page_count){
                switch_home_page(target,home_touch_app);
            }
        }else handle_click(home_touch_x,home_touch_y);
        home_touch_pending=0;
        home_touch_app=-1;
    }
    mouse_buttons = buttons;
    erase_pointer();
    draw_pointer();
}

static void poll_input(void) {
    uint8_t data;
    if (x86_ps2_mouse_read(&data)) {
        if(mouse_packet_index==0 && !(data&0x08))return; /* Packet start marker. */
        mouse_packet[mouse_packet_index++]=data;
        if(mouse_packet_index==3){mouse_packet_index=0;process_mouse_packet();}
        return;
    }
    uint8_t status=inb(0x64);
    if((status&1)&&!(status&0x20))keyboard_byte(inb(0x60));
}

static void update_clock(void) {
    /* CMOS weekday values use 1=Sunday through 7=Saturday. */
    static const char *const weekdays[] = {"", "PAZAR", "PAZARTESİ", "SALI", "ÇARŞAMBA", "PERŞEMBE", "CUMA", "CUMARTESİ"};
    static const char *const months[] = {"", "OCAK", "ŞUBAT", "MART", "NİSAN", "MAYIS", "HAZİRAN", "TEMMUZ", "AĞUSTOS", "EYLÜL", "EKİM", "KASIM", "ARALIK"};
    rtc_time_t t;
    if (!rtc_read(&t)) return;
    current_clock[0] = (char)('0' + t.hour / 10); current_clock[1] = (char)('0' + t.hour % 10);
    current_clock[2] = ':'; current_clock[3] = (char)('0' + t.minute / 10); current_clock[4] = (char)('0' + t.minute % 10); current_clock[5] = 0;
    const char *weekday = t.weekday <= 7 && t.weekday ? weekdays[t.weekday] : "";
    const char *month = months[t.month];
    size_t n=0;
    while(weekday[n]&&n+1<sizeof(current_date)){current_date[n]=weekday[n];n++;}
    if(n+2<sizeof(current_date)){current_date[n++] = ',';current_date[n++]=' ';}
    if(t.day>=10)current_date[n++]=(char)('0'+t.day/10);current_date[n++]=(char)('0'+t.day%10);current_date[n++]=' ';
    for(size_t i=0;month[i]&&n+1<sizeof(current_date);i++)current_date[n++]=month[i];
    current_date[n]=0;
}

void ui_run(void) {
    framebuffer = (volatile uint32_t *)find_framebuffer();
    mouse_x=SCREEN_W-20;
    mouse_y=SCREEN_H-64;
    uint32_t image_w=0,image_h=0;
    wallpaper_ready = png_decode(ui_wallpaper_png_start,
        (size_t)(ui_wallpaper_png_end-ui_wallpaper_png_start), wallpaper,
        sizeof(wallpaper)/sizeof(wallpaper[0]), &image_w, &image_h) &&
        image_w==LOGICAL_W && image_h==LOGICAL_H;
    uint32_t icons_w=0,icons_h=0;
    app_icons_ready=png_decode(ui_app_icons_png_start,
        (size_t)(ui_app_icons_png_end-ui_app_icons_png_start),app_icon_atlas,
        sizeof(app_icon_atlas)/sizeof(app_icon_atlas[0]),&icons_w,&icons_h) &&
        icons_w==768 && icons_h==512;
    update_clock();
    calibrate_tsc();
    render_current_screen(render_buffer);
    copy_to_display(render_buffer);
    x86_ps2_mouse_init();
    draw_pointer();
    uint32_t clock_poll=0;
    for (;;) {
        poll_input();
        if (++clock_poll == 500000) {
            clock_poll=0;
            char old[6]; for(int i=0;i<6;i++)old[i]=current_clock[i];
            update_clock();
            if(current_clock[0]!=old[0]||current_clock[1]!=old[1]||current_clock[3]!=old[3]||current_clock[4]!=old[4]) {
                redraw_current_screen();
            }
        }
        asm volatile("pause");
    }
}
