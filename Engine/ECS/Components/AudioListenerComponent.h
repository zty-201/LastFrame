#ifndef AUDIO_LISTENER_COMPONENT_H
#define AUDIO_LISTENER_COMPONENT_H

// Tag component: marks an entity as an audio listener.
// Typically attached to the player or camera entity.
struct AudioListenerComponent
{
    bool active = true;  // you can use this later if you support multiple listeners
};

#endif // AUDIO_LISTENER_COMPONENT_H