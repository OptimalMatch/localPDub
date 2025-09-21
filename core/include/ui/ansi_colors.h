#ifndef LOCALPDUB_ANSI_COLORS_H
#define LOCALPDUB_ANSI_COLORS_H

#include <string>
#include <cstdlib>
#include <unistd.h>

namespace localpdub {
namespace ui {

// ANSI Color codes
namespace ansi {
    // Reset
    const std::string RESET = "\033[0m";

    // Regular colors
    const std::string BLACK = "\033[30m";
    const std::string RED = "\033[31m";
    const std::string GREEN = "\033[32m";
    const std::string YELLOW = "\033[33m";
    const std::string BLUE = "\033[34m";
    const std::string MAGENTA = "\033[35m";
    const std::string CYAN = "\033[36m";
    const std::string WHITE = "\033[37m";

    // Bright colors
    const std::string BRIGHT_BLACK = "\033[90m";
    const std::string BRIGHT_RED = "\033[91m";
    const std::string BRIGHT_GREEN = "\033[92m";
    const std::string BRIGHT_YELLOW = "\033[93m";
    const std::string BRIGHT_BLUE = "\033[94m";
    const std::string BRIGHT_MAGENTA = "\033[95m";
    const std::string BRIGHT_CYAN = "\033[96m";
    const std::string BRIGHT_WHITE = "\033[97m";

    // Background colors
    const std::string BG_BLACK = "\033[40m";
    const std::string BG_RED = "\033[41m";
    const std::string BG_GREEN = "\033[42m";
    const std::string BG_YELLOW = "\033[43m";
    const std::string BG_BLUE = "\033[44m";
    const std::string BG_MAGENTA = "\033[45m";
    const std::string BG_CYAN = "\033[46m";
    const std::string BG_WHITE = "\033[47m";

    // Bright background colors
    const std::string BG_BRIGHT_BLACK = "\033[100m";
    const std::string BG_BRIGHT_RED = "\033[101m";
    const std::string BG_BRIGHT_GREEN = "\033[102m";
    const std::string BG_BRIGHT_YELLOW = "\033[103m";
    const std::string BG_BRIGHT_BLUE = "\033[104m";
    const std::string BG_BRIGHT_MAGENTA = "\033[105m";
    const std::string BG_BRIGHT_CYAN = "\033[106m";
    const std::string BG_BRIGHT_WHITE = "\033[107m";

    // Text styles
    const std::string BOLD = "\033[1m";
    const std::string DIM = "\033[2m";
    const std::string ITALIC = "\033[3m";
    const std::string UNDERLINE = "\033[4m";
    const std::string BLINK = "\033[5m";
    const std::string REVERSE = "\033[7m";
    const std::string HIDDEN = "\033[8m";
    const std::string STRIKETHROUGH = "\033[9m";

    // Cursor movement
    const std::string CLEAR_SCREEN = "\033[2J\033[H";
    const std::string CLEAR_LINE = "\033[2K";
    const std::string CURSOR_UP = "\033[A";
    const std::string CURSOR_DOWN = "\033[B";
    const std::string CURSOR_FORWARD = "\033[C";
    const std::string CURSOR_BACK = "\033[D";
    const std::string SAVE_CURSOR = "\033[s";
    const std::string RESTORE_CURSOR = "\033[u";
}

// Box drawing characters (CP437/Unicode)
namespace box {
    // Single line
    const std::string HORIZONTAL = "─";
    const std::string VERTICAL = "│";
    const std::string TOP_LEFT = "┌";
    const std::string TOP_RIGHT = "┐";
    const std::string BOTTOM_LEFT = "└";
    const std::string BOTTOM_RIGHT = "┘";
    const std::string CROSS = "┼";
    const std::string T_DOWN = "┬";
    const std::string T_UP = "┴";
    const std::string T_RIGHT = "├";
    const std::string T_LEFT = "┤";

    // Double line
    const std::string DOUBLE_HORIZONTAL = "═";
    const std::string DOUBLE_VERTICAL = "║";
    const std::string DOUBLE_TOP_LEFT = "╔";
    const std::string DOUBLE_TOP_RIGHT = "╗";
    const std::string DOUBLE_BOTTOM_LEFT = "╚";
    const std::string DOUBLE_BOTTOM_RIGHT = "╝";
    const std::string DOUBLE_CROSS = "╬";
    const std::string DOUBLE_T_DOWN = "╦";
    const std::string DOUBLE_T_UP = "╩";
    const std::string DOUBLE_T_RIGHT = "╠";
    const std::string DOUBLE_T_LEFT = "╣";

    // Mixed double/single
    const std::string DOUBLE_H_SINGLE_V = "╫";
    const std::string SINGLE_H_DOUBLE_V = "╪";

    // Block elements
    const std::string FULL_BLOCK = "█";
    const std::string DARK_SHADE = "▓";
    const std::string MEDIUM_SHADE = "▒";
    const std::string LIGHT_SHADE = "░";
    const std::string UPPER_HALF = "▀";
    const std::string LOWER_HALF = "▄";
    const std::string LEFT_HALF = "▌";
    const std::string RIGHT_HALF = "▐";

    // Special characters
    const std::string BULLET = "•";
    const std::string ARROW_RIGHT = "→";
    const std::string ARROW_LEFT = "←";
    const std::string ARROW_UP = "↑";
    const std::string ARROW_DOWN = "↓";
    const std::string CHECK_MARK = "✓";
    const std::string CROSS_MARK = "✗";
    const std::string STAR = "★";
    const std::string HEART = "♥";
    const std::string DIAMOND = "♦";
    const std::string CLUB = "♣";
    const std::string SPADE = "♠";
}

class AnsiUI {
private:
    static bool colors_enabled;

public:
    // Check if terminal supports colors
    static bool supportsColor() {
        // Check if output is a terminal
        if (!isatty(STDOUT_FILENO)) {
            return false;
        }

        // Check TERM environment variable
        const char* term = std::getenv("TERM");
        if (term == nullptr) {
            return false;
        }

        std::string term_str(term);
        if (term_str == "dumb" || term_str == "") {
            return false;
        }

        // Check for NO_COLOR environment variable
        if (std::getenv("NO_COLOR") != nullptr) {
            return false;
        }

        return true;
    }

    // Enable or disable colors
    static void setColorsEnabled(bool enabled) {
        colors_enabled = enabled;
    }

    // Get color string (returns empty if colors disabled)
    static std::string color(const std::string& colorCode) {
        return colors_enabled ? colorCode : "";
    }

    // Convenience functions for colored text
    static std::string red(const std::string& text) {
        return color(ansi::RED) + text + color(ansi::RESET);
    }

    static std::string green(const std::string& text) {
        return color(ansi::GREEN) + text + color(ansi::RESET);
    }

    static std::string yellow(const std::string& text) {
        return color(ansi::YELLOW) + text + color(ansi::RESET);
    }

    static std::string blue(const std::string& text) {
        return color(ansi::BLUE) + text + color(ansi::RESET);
    }

    static std::string magenta(const std::string& text) {
        return color(ansi::MAGENTA) + text + color(ansi::RESET);
    }

    static std::string cyan(const std::string& text) {
        return color(ansi::CYAN) + text + color(ansi::RESET);
    }

    static std::string white(const std::string& text) {
        return color(ansi::WHITE) + text + color(ansi::RESET);
    }

    static std::string bold(const std::string& text) {
        return color(ansi::BOLD) + text + color(ansi::RESET);
    }

    static std::string success(const std::string& text) {
        return color(ansi::BRIGHT_GREEN) + "✓ " + text + color(ansi::RESET);
    }

    static std::string error(const std::string& text) {
        return color(ansi::BRIGHT_RED) + "✗ " + text + color(ansi::RESET);
    }

    static std::string warning(const std::string& text) {
        return color(ansi::BRIGHT_YELLOW) + "⚠ " + text + color(ansi::RESET);
    }

    static std::string info(const std::string& text) {
        return color(ansi::BRIGHT_CYAN) + "ℹ " + text + color(ansi::RESET);
    }

    // Draw a box with title
    static std::string drawBox(const std::string& title, int width = 50) {
        std::string result;

        // Top border
        result += color(ansi::BRIGHT_CYAN);
        result += box::DOUBLE_TOP_LEFT;
        for (int i = 0; i < width - 2; ++i) {
            result += box::DOUBLE_HORIZONTAL;
        }
        result += box::DOUBLE_TOP_RIGHT + "\n";

        // Title line
        result += box::DOUBLE_VERTICAL;
        result += color(ansi::BRIGHT_WHITE) + color(ansi::BOLD);
        int padding = (width - 2 - title.length()) / 2;
        for (int i = 0; i < padding; ++i) result += " ";
        result += title;
        for (int i = 0; i < width - 2 - padding - title.length(); ++i) result += " ";
        result += color(ansi::BRIGHT_CYAN) + box::DOUBLE_VERTICAL + "\n";

        // Bottom border
        result += box::DOUBLE_BOTTOM_LEFT;
        for (int i = 0; i < width - 2; ++i) {
            result += box::DOUBLE_HORIZONTAL;
        }
        result += box::DOUBLE_BOTTOM_RIGHT;
        result += color(ansi::RESET);

        return result;
    }

    // Draw a progress bar
    static std::string progressBar(int percentage, int width = 30) {
        std::string result = "[";
        int filled = (percentage * width) / 100;

        result += color(ansi::BRIGHT_GREEN);
        for (int i = 0; i < filled; ++i) {
            result += box::FULL_BLOCK;
        }

        result += color(ansi::DIM);
        for (int i = filled; i < width; ++i) {
            result += box::LIGHT_SHADE;
        }
        result += color(ansi::RESET);

        result += "] " + std::to_string(percentage) + "%";
        return result;
    }

    // Rainbow text (for fun BBS-style effects)
    static std::string rainbow(const std::string& text) {
        std::string result;
        const std::string colors[] = {
            ansi::RED, ansi::YELLOW, ansi::GREEN,
            ansi::CYAN, ansi::BLUE, ansi::MAGENTA
        };

        for (size_t i = 0; i < text.length(); ++i) {
            result += color(colors[i % 6]) + text[i];
        }
        result += color(ansi::RESET);
        return result;
    }

    // Gradient effect
    static std::string gradient(const std::string& text, const std::string& startColor, const std::string& endColor) {
        // Simple two-color gradient
        std::string result;
        size_t mid = text.length() / 2;

        for (size_t i = 0; i < mid; ++i) {
            result += color(startColor) + text[i];
        }
        for (size_t i = mid; i < text.length(); ++i) {
            result += color(endColor) + text[i];
        }
        result += color(ansi::RESET);
        return result;
    }
};

// Initialize static member
bool AnsiUI::colors_enabled = AnsiUI::supportsColor();

} // namespace ui
} // namespace localpdub

#endif // LOCALPDUB_ANSI_COLORS_H