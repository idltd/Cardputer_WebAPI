#pragma once

void setupIrApi();

// Wrapper for serial API to send IR without including TinyIRSender.hpp
// Returns true if protocol is recognized, false otherwise.
bool irSend(const char* protocol, uint16_t address, uint8_t command, int repeats);
