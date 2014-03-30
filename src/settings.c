#include <pebble.h>
#include <settings.h>
#include <calendarUtils.h>
#include <calendarWindow.h>
#include <agendaWindow.h>
#include "pebble_app_info.h"

extern const PebbleAppInfo __pbl_app_info;

bool watchmode =  false;

bool black = true;
bool grid = true;
bool invert = true;
bool showtime = true;
bool hidelastprev = true;
bool boldevents = true;

// First day of the week. Values can be between -6 and 6 
// 0 = weeks start on Sunday
// 1 =  weeks start on Monday
int start_of_week = 0;
int showweekno = 0;
int weekstoshow = 0;
int agenda_title_rows = 2;

char weekno_form[4][3] = {"","%V","%U","%W"};
char daysOfWeek[7][3] = {"S","M","T","W","Th","F","S"};
char months[12][12] = {"January","Feburary","March","April","May","June","July","August","September","October", "November", "December"};

int offset = 0;


int curHour;
int curMin;
int curSec;

bool calEvents[32] = {  false,false,false,false,false,
                        false,false,false,false,false,
                        false,false,false,false,false,
                        false,false,false,false,false,
                        false,false,false,false,false,
                        false,false,false,false,false,false,false};

int agendaLength = -1;
char agenda[MAX_AGENDA_LENGTH][3][MAX_AGENDA_TITLE];

void getmode(){
    watchmode = __pbl_app_info.flags==RESOURCE_ID_IMAGE_MENU_ICON;
}
void readSettings(){


    if( persist_exists(BLACK_KEY))          black =         persist_read_bool(BLACK_KEY);
    if( persist_exists(GRID_KEY))           grid =          persist_read_bool(GRID_KEY);
    if( persist_exists(INVERT_KEY))         invert =        persist_read_bool(INVERT_KEY);
    if( persist_exists(SHOWTIME_KEY))       showtime =      persist_read_bool(SHOWTIME_KEY);
    if( persist_exists(HIDELASTPREV_KEY))   hidelastprev =  persist_read_bool(HIDELASTPREV_KEY);
    if( persist_exists(BOLDEVENTS_KEY))     boldevents =    persist_read_bool(BOLDEVENTS_KEY);
    if( persist_exists(START_OF_WEEK_KEY))  start_of_week = persist_read_int(START_OF_WEEK_KEY);
    if( persist_exists(SHOWWEEKNO_KEY))     showweekno =    persist_read_int(SHOWWEEKNO_KEY);
    if( persist_exists(WEEKSTOSHOW_KEY))    weekstoshow =   persist_read_int(WEEKSTOSHOW_KEY);
    if( persist_exists(AGENDA_KEY))         agendaLength =  persist_read_int(AGENDA_KEY);
    if( persist_exists(AGENDA_TITLE_KEY))   agenda_title_rows =  persist_read_int(AGENDA_TITLE_KEY);
    
    for(int i = 0; i<MAX_AGENDA_LENGTH;i++ ){
        if( persist_exists(AGENDA_KEY+i+1))
            persist_read_data(AGENDA_KEY+i+1,agenda[i],sizeof(agenda[i]));
    }
}

void get_settings(){
    DictionaryIterator *iter;
        
    time_t now = time(NULL);
    struct tm *currentTime = localtime(&now);
        
    int year = currentTime->tm_year;
    int month = currentTime->tm_mon+offset ;
    
    while(month>11){
        month -= 12;
        year++;
    }
    while(month<0){
        month += 12;
        year--;
    }
    
    if (app_message_outbox_begin(&iter) != APP_MSG_OK) {
        app_log(APP_LOG_LEVEL_DEBUG, "calendarApp.c",364,"App MSG Not ok");
        return;
    }    
    if (dict_write_uint8(iter, GET_SETTINGS, ((uint8_t)0)) != DICT_OK) {
        app_log(APP_LOG_LEVEL_DEBUG, "calendarApp.c",364,"Dict Not ok");
        return;
    }
    if(year*100+month>0){
        if (dict_write_uint16(iter, GET_EVENT_DAYS, ((uint16_t)year*100+month)) != DICT_OK) {
            app_log(APP_LOG_LEVEL_DEBUG, "calendarApp.c",364,"Dict Not ok");
            return;
        }
    }
    if (app_message_outbox_send() != APP_MSG_OK){
        app_log(APP_LOG_LEVEL_DEBUG, "calendarApp.c",364,"Message Not Sent");
        return;
    }
    app_log(APP_LOG_LEVEL_DEBUG, "calendarApp.c",364,"Message Sent");
}

void processSettings(uint8_t encoded[4]){

    black               = (encoded[0] & (1 << 0)) != 0;
    grid                = (encoded[0] & (1 << 1)) != 0;
    invert              = (encoded[0] & (1 << 2)) != 0;
    showtime            = (encoded[0] & (1 << 3)) != 0;
    hidelastprev        = (encoded[0] & (1 << 4)) != 0;
    boldevents          = (encoded[0] & (1 << 5)) != 0;

    start_of_week       = (int) encoded[1];
    showweekno          = (int) encoded[2];
    weekstoshow         = (int) encoded[3];
    agenda_title_rows   = (int) encoded[4]+1;

    int changed = false;
    if( ( ! persist_exists(BLACK_KEY)         ) || persist_read_bool(BLACK_KEY)         != black            ){ persist_write_bool(BLACK_KEY,         black);         changed = true;}
    if( ( ! persist_exists(GRID_KEY)          ) || persist_read_bool(GRID_KEY)          != grid             ){ persist_write_bool(GRID_KEY,          grid);          changed = true;}
    if( ( ! persist_exists(INVERT_KEY)        ) || persist_read_bool(INVERT_KEY)        != invert           ){ persist_write_bool(INVERT_KEY,        invert);        changed = true;}
    if( ( ! persist_exists(SHOWTIME_KEY)      ) || persist_read_bool(SHOWTIME_KEY)      != showtime         ){ persist_write_bool(SHOWTIME_KEY,      showtime);      changed = true;}
    if( ( ! persist_exists(HIDELASTPREV_KEY)  ) || persist_read_bool(HIDELASTPREV_KEY)  != hidelastprev     ){ persist_write_bool(HIDELASTPREV_KEY,  hidelastprev);  changed = true;}
    if( ( ! persist_exists(BOLDEVENTS_KEY)    ) || persist_read_bool(BOLDEVENTS_KEY)    != boldevents       ){ persist_write_bool(BOLDEVENTS_KEY,    boldevents);    changed = true;}
    if( ( ! persist_exists(START_OF_WEEK_KEY) ) || persist_read_int (START_OF_WEEK_KEY) != start_of_week    ){ persist_write_int(START_OF_WEEK_KEY,  start_of_week); changed = true;}
    if( ( ! persist_exists(SHOWWEEKNO_KEY)    ) || persist_read_int (SHOWWEEKNO_KEY)    != showweekno       ){ persist_write_int(SHOWWEEKNO_KEY,     showweekno);    changed = true;}
    if( ( ! persist_exists(WEEKSTOSHOW_KEY)   ) || persist_read_int (WEEKSTOSHOW_KEY)   != weekstoshow      ){ persist_write_int(WEEKSTOSHOW_KEY,    weekstoshow);   changed = true;}
    if( ( ! persist_exists(AGENDA_TITLE_KEY)  ) || persist_read_int (AGENDA_TITLE_KEY)  != agenda_title_rows){ persist_write_int(AGENDA_TITLE_KEY,    agenda_title_rows);  agenda_mark_dirty(); }



    if(changed){
    
        if(black)
            window_set_background_color(calendar_window, GColorBlack);
        else
            window_set_background_color(calendar_window, GColorWhite);
        
        
        if(showtime){
        
            if(black)
                text_layer_set_text_color(timeLayer, GColorWhite);
            else
                text_layer_set_text_color(timeLayer, GColorBlack);
            time_t now = time(NULL);
            struct tm *currentTime = localtime(&now);
            updateTime(currentTime);
        }
        else{
            text_layer_set_text(timeLayer, "");
        }
        layer_mark_dirty(days_layer);
    }
}


