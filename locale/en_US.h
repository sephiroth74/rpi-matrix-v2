#ifndef LOCALE_EN_US_H
#define LOCALE_EN_US_H

// English (US) localization

namespace Locale {
    // Day names (short form)
    const char* DAY_NAMES[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

    // Month names (short form)
    const char* MONTH_NAMES[] = {
        "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
        "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
    };

    // UI Messages
    const char* MSG_AUTO = "AUTO";
    const char* MSG_VERSION_PREFIX = "v";

    // Date/time format
    const char* DATE_FORMAT = "%s %d %s";  // "MON 15 DEC"
    const char* TIME_FORMAT = "%H:%M:%S";   // "14:30:45"
}

#endif // LOCALE_EN_US_H
