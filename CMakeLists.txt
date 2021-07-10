add_executable(PicoRemoteDeck
        PicoRemoteDeck.c
        )

target_link_libraries(PicoRemoteDeck pico_stdlib)

pico_add_extra_outputs(PicoRemoteDeck)