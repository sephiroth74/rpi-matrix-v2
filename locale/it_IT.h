#ifndef LOCALE_IT_IT_H
#define LOCALE_IT_IT_H

// Italian localization

namespace Locale {
    // Day names (short form)
    const char* DAY_NAMES[] = {"DOM", "LUN", "MAR", "MER", "GIO", "VEN", "SAB"};

    // Month names (short form)
    const char* MONTH_NAMES[] = {
        "GEN", "FEB", "MAR", "APR", "MAG", "GIU",
        "LUG", "AGO", "SET", "OTT", "NOV", "DIC"
    };

    // UI Messages
    const char* MSG_AUTO = "AUTO";
    const char* MSG_VERSION_PREFIX = "v";

    // Date/time format
    const char* DATE_FORMAT = "%s %d %s";  // "LUN 15 DIC"
    const char* TIME_FORMAT = "%H:%M:%S";   // "14:30:45"
}

#endif // LOCALE_IT_IT_H
