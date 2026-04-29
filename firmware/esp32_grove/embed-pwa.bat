@echo off
:: Embeds pwa/index.html into api_webapp.cpp as a C string literal.
:: Run this after editing the PWA.
py embed_pwa.py
echo Done.
