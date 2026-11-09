#!/usr/bin/env python3
"""Generate the example .ddesign files shipped with DeepDesign Studio.

The file format is plain JSON; this script is a convenient way to author the
samples and doubles as executable documentation of the format.
"""
from __future__ import annotations

import json
import os
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EXAMPLES = ROOT / "examples"
EXAMPLES.mkdir(exist_ok=True)


def base(element_id, kind, name, x, y, w, h, **extra):
    data = {
        "id": element_id,
        "type": kind,
        "name": name,
        "x": round(x, 2),
        "y": round(y, 2),
        "width": round(w, 2),
        "height": round(h, 2),
        "rotation": 0,
        "opacity": 1,
        "fill": "#2D8CEB",
        "stroke": "#2D8CEB",
        "strokeWidth": 0,
        "visible": True,
        "locked": False,
    }
    data.update(extra)
    return data


def rect(element_id, name, x, y, w, h, fill, radius=0, stroke=None, sw=0, opacity=1, rotation=0):
    return base(element_id, "rect", name, x, y, w, h, fill=fill, radius=radius,
                stroke=stroke or fill, strokeWidth=sw, opacity=opacity, rotation=rotation)


def ellipse(element_id, name, x, y, w, h, fill, stroke=None, sw=0, opacity=1, rotation=0):
    return base(element_id, "ellipse", name, x, y, w, h, fill=fill,
                stroke=stroke or fill, strokeWidth=sw, opacity=opacity, rotation=rotation)


def line(element_id, name, x, y, w, h, stroke, sw=1, start=(0, 0), end=(1, 1), opacity=1):
    return base(element_id, "line", name, x, y, w, h, fill=stroke, stroke=stroke,
                strokeWidth=sw, lineStart=list(start), lineEnd=list(end), opacity=opacity)


def polygon(element_id, name, x, y, w, h, sides, fill, stroke=None, sw=0, opacity=1, rotation=0):
    return base(element_id, "polygon", name, x, y, w, h, fill=fill, stroke=stroke or fill,
                strokeWidth=sw, sides=sides, opacity=opacity, rotation=rotation)


def star(element_id, name, x, y, w, h, fill, stroke=None, sw=0, opacity=1, rotation=0, sides=5):
    return base(element_id, "star", name, x, y, w, h, fill=fill, stroke=stroke or fill,
                strokeWidth=sw, sides=sides, opacity=opacity, rotation=rotation)


def path(element_id, name, x, y, w, h, points, fill, stroke=None, sw=0, opacity=1, closed=True):
    return base(element_id, "path", name, x, y, w, h, points=points, closed=closed,
                fill=fill, stroke=stroke or fill, strokeWidth=sw, opacity=opacity)


def with_gradient(element, kind, angle, stops):
    element["gradient"] = {
        "type": kind,
        "angle": angle,
        "stops": [{"position": position, "color": color} for position, color in stops],
    }
    return element


def text(element_id, name, x, y, w, h, content, size=16, color="#E8ECF6",
         family="Microsoft YaHei UI", bold=False, align="left", opacity=1):
    return base(element_id, "text", name, x, y, w, h, text=content, fontFamily=family,
                fontSize=size, bold=bold, align=align, textColor=color, opacity=opacity,
                fill="#00000000", stroke="#00000000", strokeWidth=0)


def key(time, value):
    return {"time": round(time, 3), "value": value}


def track(element_id, prop, keys):
    return {"elementId": element_id, "property": prop, "keyframes": keys}


def animation(element_id, name, duration, easing, loop, tracks):
    return {
        "id": element_id,
        "name": name,
        "duration": duration,
        "loop": loop,
        "easing": easing,
        "tracks": tracks,
    }


def interaction(source, trigger, animation_id, action="playAnimation"):
    data = {
        "sourceId": source,
        "trigger": trigger,
        "action": action,
    }
    if animation_id:
        data["animationId"] = animation_id
    return data


def document(title, width, height, background, elements, animations, interactions):
    return {
        "format": "DeepDesign",
        "version": 1,
        "generator": "DeepDesign Studio 1.0 (examples generator)",
        "title": title,
        "canvas": {"width": width, "height": height, "background": background},
        "elements": elements,
        "animations": animations,
        "interactions": interactions,
    }


# ---------------------------------------------------------------------------
# 1. Login screen
# ---------------------------------------------------------------------------
def login():
    els = [
        ellipse("glow_blue", "蓝色光晕", -170, -190, 650, 650, "#12335F", opacity=0.55),
        ellipse("glow_violet", "紫色光晕", 900, 480, 500, 500, "#2B1A4A", opacity=0.55),
        path("wave", "底部波纹", 0, 620, 1280, 180,
             [(0, 0.55), (0.16, 0.25), (0.32, 0.48), (0.5, 0.18),
              (0.68, 0.42), (0.84, 0.2), (1, 0.5), (1, 1), (0, 1)],
             "#10233A", opacity=0.7),
        star("deco_star_1", "装饰星", 1030, 170, 44, 44, "#5AA9F0", opacity=0.5, rotation=12),
        star("deco_star_2", "装饰星 2", 250, 610, 26, 26, "#A47BFF", opacity=0.6, rotation=-12),
        polygon("deco_hex", "装饰六边形", 1060, 640, 92, 92, 6, "#1A2B46", opacity=0.8, rotation=18),

        with_gradient(rect("login_card", "登录卡片", 380, 90, 520, 620, "#121A2B", radius=28,
                           stroke="#23304A", sw=1.5),
                      "linear", 135, [(0.0, "#1A2740"), (0.6, "#111A2A"), (1.0, "#0D1421")]),
        with_gradient(rect("logo_card", "品牌标记", 604, 126, 72, 72, "#2D8CEB", radius=18,
                           stroke="#5AA9F0", sw=1),
                      "linear", 135, [(0.0, "#5AA9F0"), (1.0, "#2D8CEB")]),
        text("logo_letter", "品牌字母", 604, 126, 72, 72, "D", 36, "#FFFFFF", bold=True, align="center"),

        text("title", "欢迎标题", 380, 232, 520, 52, "欢迎回来", 36, "#F4F7FF",
             bold=True, align="center"),
        text("subtitle", "副标题", 380, 292, 520, 28, "登录以继续使用灵绘云设计工作台", 14,
             "#8B97AC", align="center"),

        text("email_label", "邮箱标签", 450, 334, 200, 22, "邮箱地址", 12, "#7E8CA3"),
        rect("email_input", "邮箱输入框", 450, 360, 380, 54, "#0A1019", radius=12,
             stroke="#2A3854", sw=1),
        ellipse("email_dot", "邮箱状态点", 466, 382, 8, 8, "#3DDC97"),
        text("email_text", "邮箱内容", 486, 360, 322, 54, "hello@deepdesign.ai", 14,
             "#C7D2E3", align="left"),

        text("password_label", "密码标签", 450, 430, 200, 22, "密码", 12, "#7E8CA3"),
        rect("password_input", "密码输入框", 450, 456, 380, 54, "#0A1019", radius=12,
             stroke="#2A3854", sw=1),
        text("password_text", "密码占位", 470, 456, 330, 54, "••••••••••", 15,
             "#C7D2E3", align="left"),
        text("show_password", "显示密码", 738, 456, 76, 54, "显示", 12, "#5AA9F0",
             align="right"),

        rect("login_button", "登录按钮", 450, 548, 380, 58, "#2D8CEB", radius=14,
             stroke="#70A9F2", sw=1),
        text("login_button_text", "登录按钮文字", 450, 548, 380, 58, "登 录", 17,
             "#FFFFFF", bold=True, align="center"),
        text("forgot_password", "忘记密码", 690, 620, 140, 22, "忘记密码？", 12,
             "#7E8CA3", align="right"),
        line("divider", "分隔线", 450, 654, 380, 1, "#26324A", 1, (0, 0), (1, 0)),
        text("register_hint", "注册提示", 380, 672, 520, 24, "还没有账号？ 立即注册", 12,
             "#8B97AC", align="center"),
    ]

    anim_intro = animation("anim_intro", "入场动画", 1.2, "easeOutCubic", False, [
        track("login_card", "opacity", [key(0, 0), key(0.55, 1)]),
        track("login_card", "y", [key(0, 118), key(0.7, 90)]),
        track("logo_card", "rotation", [key(0, -90), key(0.75, 0)]),
        track("logo_card", "opacity", [key(0, 0), key(0.45, 1)]),
        track("logo_letter", "opacity", [key(0, 0), key(0.55, 1)]),
        track("title", "y", [key(0.1, 266), key(0.75, 232)]),
        track("title", "opacity", [key(0.1, 0), key(0.75, 1)]),
        track("subtitle", "y", [key(0.18, 324), key(0.85, 292)]),
        track("subtitle", "opacity", [key(0.18, 0), key(0.85, 1)]),
        track("email_input", "x", [key(0.3, 420), key(0.9, 450)]),
        track("email_input", "opacity", [key(0.3, 0), key(0.9, 1)]),
        track("password_input", "x", [key(0.38, 410), key(0.98, 450)]),
        track("password_input", "opacity", [key(0.38, 0), key(0.98, 1)]),
        track("login_button", "y", [key(0.45, 580), key(1.05, 548)]),
        track("login_button", "opacity", [key(0.45, 0), key(1.05, 1)]),
        track("login_button_text", "opacity", [key(0.55, 0), key(1.1, 1)]),
    ])

    anim_hover = animation("anim_login_hover", "按钮悬停", 0.32, "easeOutCubic", False, [
        track("login_button", "fill", [key(0, "#2D8CEB"), key(0.32, "#3D9CFA")]),
        track("login_button", "x", [key(0, 450), key(0.32, 446)]),
        track("login_button", "width", [key(0, 380), key(0.32, 388)]),
        track("login_button", "y", [key(0, 548), key(0.32, 544)]),
        track("login_button", "height", [key(0, 58), key(0.32, 66)]),
    ])

    anim_click = animation("anim_login_click", "登录点击反馈", 0.55, "easeInOutCubic", False, [
        track("login_button", "fill", [key(0, "#2D8CEB"), key(0.22, "#3DDC97"),
                                       key(0.36, "#3DDC97"), key(0.55, "#2D8CEB")]),
        track("login_card", "rotation", [key(0, 0), key(0.18, -0.8), key(0.36, 0.8), key(0.55, 0)]),
    ])

    interactions = [
        interaction("login_card", "load", "anim_intro"),
        interaction("login_button", "hover", "anim_login_hover"),
        interaction("login_button", "click", "anim_login_click"),
    ]
    return document("登录界面 · 极光蓝", 1280, 800, "#0A0F1A", els,
                    [anim_intro, anim_hover, anim_click], interactions)


# ---------------------------------------------------------------------------
# 2. Music player
# ---------------------------------------------------------------------------
def music_player():
    els = [
        ellipse("bg_glow_1", "背景光晕 1", -220, -240, 700, 700, "#13325C", opacity=0.5),
        ellipse("bg_glow_2", "背景光晕 2", 900, 420, 620, 620, "#3B1F4E", opacity=0.45),
        star("bg_star", "背景星点", 1110, 130, 60, 60, "#FFB454", opacity=0.55, rotation=14),

        with_gradient(rect("player_card", "播放器主卡片", 100, 86, 1080, 628, "#111826", radius=30,
                           stroke="#1F2B40", sw=1.5),
                      "linear", 135, [(0.0, "#151F30"), (1.0, "#0C111C")]),
        with_gradient(rect("album_card", "专辑封面卡片", 136, 122, 396, 556, "#161F2F", radius=24,
                           stroke="#24324A", sw=1),
                      "linear", 90, [(0.0, "#1B2638"), (1.0, "#101724")]),
        with_gradient(rect("cover", "专辑封面", 164, 150, 340, 340, "#1B2A45", radius=20,
                           stroke="#2D8CEB", sw=1.5),
                      "linear", 135, [(0.0, "#233A5C"), (1.0, "#0F1725")]),
        ellipse("vinyl", "唱片", 204, 190, 260, 260, "#2D8CEB", rotation=0),
        ellipse("vinyl_ring", "唱片纹路", 236, 222, 196, 196, "#0F1725",
                stroke="#FFFFFF", sw=3, opacity=0.95),
        ellipse("vinyl_hole", "唱片中心", 300, 286, 68, 68, "#EAF3FF"),
        text("album_badge", "专辑标签", 164, 506, 180, 22, "NOW PLAYING", 11, "#5AA9F0",
             bold=True, align="left"),
        text("album_title", "专辑标题", 164, 530, 340, 36, "AURORA NIGHTS", 23, "#F4F7FF",
             bold=True, align="left"),
        text("album_artist", "专辑艺人", 164, 566, 340, 24, "DeepSeek Waves", 13,
             "#8B97AC", align="left"),
        rect("progress_bg", "进度条背景", 164, 612, 340, 4, "#26324A", radius=2),
        rect("progress", "播放进度", 164, 612, 148, 4, "#2D8CEB", radius=2),
        text("time_now", "已播放时间", 164, 626, 60, 20, "0:48", 11, "#7E8CA3"),
        text("time_total", "总时长", 444, 626, 60, 20, "3:12", 11, "#7E8CA3", align="right"),

        text("playing_label", "播放状态", 620, 140, 300, 22, "正在播放", 12, "#5AA9F0",
             bold=True, align="left"),
        text("track_title", "歌曲标题", 620, 166, 540, 48, "Aurora Nights", 36, "#F4F7FF",
             bold=True, align="left"),
        text("track_artist", "歌曲艺人", 620, 214, 540, 26, "DeepSeek Waves · 氛围电子", 15,
             "#8B97AC", align="left"),
        star("like_star", "收藏星标", 1110, 178, 52, 52, "#FFB454", opacity=0.9, rotation=8),

        rect("playlist_card", "播放列表", 620, 260, 540, 350, "#0D1420", radius=22,
             stroke="#1E2A3D", sw=1),
        rect("active_row", "当前播放行", 638, 278, 504, 58, "#16233A", radius=14),
        ellipse("active_dot", "播放指示点", 656, 300, 10, 10, "#2D8CEB"),
        text("track_1", "曲目 1", 682, 286, 330, 42, "Aurora Nights", 15, "#F4F7FF",
             bold=True, align="left"),
        text("track_1_artist", "曲目 1 艺人", 682, 310, 330, 20, "DeepSeek Waves", 11,
             "#7E8CA3", align="left"),
        text("track_1_time", "曲目 1 时长", 1064, 288, 60, 40, "3:12", 11, "#8B97AC",
             align="right"),

        text("track_2", "曲目 2", 682, 350, 300, 34, "Neon Tides", 14, "#C7D2E3", align="left"),
        text("track_2_artist", "曲目 2 艺人", 682, 376, 300, 20, "Mirage Unit", 11, "#6F7B90",
             align="left"),
        text("track_2_time", "曲目 2 时长", 1064, 358, 60, 30, "4:05", 11, "#6F7B90",
             align="right"),
        line("row_sep_2", "分隔线 2", 656, 410, 468, 1, "#1B2536", 1, (0, 0), (1, 0)),

        text("track_3", "曲目 3", 682, 426, 300, 34, "Glass Horizon", 14, "#C7D2E3", align="left"),
        text("track_3_artist", "曲目 3 艺人", 682, 452, 300, 20, "Lumen Field", 11, "#6F7B90",
             align="left"),
        text("track_3_time", "曲目 3 时长", 1064, 434, 60, 30, "3:48", 11, "#6F7B90",
             align="right"),
        line("row_sep_3", "分隔线 3", 656, 486, 468, 1, "#1B2536", 1, (0, 0), (1, 0)),

        text("track_4", "曲目 4", 682, 502, 300, 34, "Silent Orbit", 14, "#C7D2E3", align="left"),
        text("track_4_artist", "曲目 4 艺人", 682, 528, 300, 20, "DeepSeek Waves", 11, "#6F7B90",
             align="left"),
        text("track_4_time", "曲目 4 时长", 1064, 510, 60, 30, "5:21", 11, "#6F7B90",
             align="right"),

        ellipse("ctrl_prev", "上一首按钮", 640, 650, 54, 54, "#182234", stroke="#2A3854", sw=1),
        polygon("ctrl_prev_icon", "上一首图标", 653, 663, 28, 28, 3, "#C7D2E3", rotation=-90),
        ellipse("play_button", "播放按钮", 726, 636, 82, 82, "#2D8CEB", stroke="#70A9F2", sw=1),
        polygon("play_icon", "播放图标", 754, 661, 30, 34, 3, "#FFFFFF", rotation=90),
        ellipse("ctrl_next", "下一首按钮", 840, 650, 54, 54, "#182234", stroke="#2A3854", sw=1),
        polygon("ctrl_next_icon", "下一首图标", 853, 663, 28, 28, 3, "#C7D2E3", rotation=90),

        rect("eq_1", "频谱柱 1", 934, 676, 8, 24, "#2D8CEB", radius=4),
        rect("eq_2", "频谱柱 2", 950, 664, 8, 36, "#5AA9F0", radius=4),
        rect("eq_3", "频谱柱 3", 966, 682, 8, 18, "#A47BFF", radius=4),
        text("eq_label", "频谱标签", 998, 668, 120, 30, "LIVE MIX", 11, "#5AA9F0",
             bold=True, align="left"),
    ]

    anim_intro = animation("anim_music_intro", "播放器入场", 1.25, "easeOutCubic", False, [
        track("player_card", "opacity", [key(0, 0), key(0.65, 1)]),
        track("player_card", "y", [key(0, 116), key(0.8, 86)]),
        track("album_card", "opacity", [key(0.08, 0), key(0.8, 1)]),
        track("album_card", "x", [key(0.08, 108), key(0.8, 136)]),
        track("cover", "rotation", [key(0.15, -8), key(0.9, 0)]),
        track("track_title", "x", [key(0.2, 660), key(0.95, 620)]),
        track("track_title", "opacity", [key(0.2, 0), key(0.95, 1)]),
        track("track_artist", "opacity", [key(0.3, 0), key(1.0, 1)]),
        track("playlist_card", "opacity", [key(0.3, 0), key(1.05, 1)]),
        track("playlist_card", "y", [key(0.3, 286), key(1.05, 260)]),
        track("play_button", "opacity", [key(0.5, 0), key(1.15, 1)]),
        track("ctrl_prev", "opacity", [key(0.55, 0), key(1.2, 1)]),
        track("ctrl_next", "opacity", [key(0.55, 0), key(1.2, 1)]),
    ])

    anim_pulse = animation("anim_play_pulse", "播放脉冲", 1.6, "easeInOutCubic", True, [
        track("play_button", "fill", [key(0, "#2D8CEB"), key(0.8, "#3DDC97"), key(1.6, "#2D8CEB")]),
        track("progress", "width", [key(0, 148), key(1.6, 340)]),
        track("vinyl_ring", "rotation", [key(0, 0), key(1.6, 360)]),
    ])

    anim_equalizer = animation("anim_equalizer", "频谱律动", 0.8, "easeInOutCubic", True, [
        track("eq_1", "height", [key(0, 10), key(0.2, 34), key(0.4, 14), key(0.6, 40), key(0.8, 10)]),
        track("eq_1", "y", [key(0, 690), key(0.2, 666), key(0.4, 686), key(0.6, 660), key(0.8, 690)]),
        track("eq_2", "height", [key(0, 30), key(0.2, 12), key(0.4, 38), key(0.6, 20), key(0.8, 30)]),
        track("eq_2", "y", [key(0, 670), key(0.2, 688), key(0.4, 662), key(0.6, 680), key(0.8, 670)]),
        track("eq_3", "height", [key(0, 18), key(0.2, 42), key(0.4, 16), key(0.6, 32), key(0.8, 18)]),
        track("eq_3", "y", [key(0, 682), key(0.2, 658), key(0.4, 684), key(0.6, 668), key(0.8, 682)]),
    ])

    anim_hover = animation("anim_cover_hover", "封面悬停", 0.4, "easeOutBack", False, [
        track("cover", "x", [key(0, 164), key(0.4, 158)]),
        track("cover", "y", [key(0, 150), key(0.4, 144)]),
        track("cover", "width", [key(0, 340), key(0.4, 352)]),
        track("cover", "height", [key(0, 340), key(0.4, 352)]),
        track("like_star", "rotation", [key(0, 8), key(0.4, 28)]),
    ])

    interactions = [
        interaction("player_card", "load", "anim_music_intro"),
        interaction("cover", "hover", "anim_cover_hover"),
        interaction("play_button", "click", "anim_play_pulse"),
        interaction("play_button", "click", "anim_equalizer"),
        interaction("ctrl_next", "click", "anim_play_pulse"),
    ]
    return document("音乐播放器 · Aurora", 1280, 800, "#0A0D14", els,
                    [anim_intro, anim_pulse, anim_equalizer, anim_hover], interactions)


def write(name, data):
    path = EXAMPLES / name
    with path.open("w", encoding="utf-8") as handle:
        json.dump(data, handle, ensure_ascii=False, indent=2)
        handle.write("\n")
    print("wrote", path)


if __name__ == "__main__":
    write("login.ddesign", login())
    write("music-player.ddesign", music_player())
