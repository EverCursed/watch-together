#ifndef COMMUNICATION_MESSAGES_H
#define COMMUNICATION_MESSAGES_H

// ------------ CONTROL MESSAGES -----------------

enum NetworkMessageID {
    // NOTE(Val): Standard initiation messages
    MESSAGE_REQUEST_INIT,
    MESSAGE_INIT,
    MESSAGE_FINISH_INIT,
    MESSAGE_READY_PLAYBACK,
    
    // NOTE(Val): Messages can be sent at any time
 MESSAGE_REQUEST_PORT,
        MESSAGE_INFO,
        MESSAGE_PAUSE,
 MESSAGE_PLAY,
 MESSAGE_SEEK,
 MESSAGE_DISCONNECT,
    
    MESSAGE_PROBE_RTT,
    MESSAGE_REPLY_RTT,
    
    // NOTE(Val): Future messages that will be added
    REQUEST_SUBTITLES,
    REQUEST_CANCEL_SUBTITLES,
REQUEST_AUDIO_CHANGE,
    MESSAGE_CHAT,
        MESSAGE_SCREENSHOT,
};

// NOTE(Val): Not sure if will use these ones yet.


#endif // COMMUNICATION_MESSAGES_H
