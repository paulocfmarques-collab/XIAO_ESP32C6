.DEFAULT_GOAL := ajuda

POWERSHELL ?= powershell.exe
JOBS ?= 1
PORT ?=

.PHONY: ajuda preparar compilar portas usb ota

ajuda:
	@echo "Alvos: preparar compilar portas usb ota"
	@echo "Exemplo: make usb PORT=COM5"
	@echo "Exemplo: make ota PORT=192.168.1.50"

preparar:
	@$(POWERSHELL) -NoProfile -ExecutionPolicy Bypass -File firmware.ps1 -Acao preparar

compilar:
	@$(POWERSHELL) -NoProfile -ExecutionPolicy Bypass -File firmware.ps1 -Acao compilar -Jobs "$(JOBS)"

portas:
	@$(POWERSHELL) -NoProfile -ExecutionPolicy Bypass -File firmware.ps1 -Acao portas

usb:
	@$(POWERSHELL) -NoProfile -ExecutionPolicy Bypass -File firmware.ps1 -Acao usb -Porta "$(PORT)" -Jobs "$(JOBS)"

ota:
	@$(POWERSHELL) -NoProfile -ExecutionPolicy Bypass -File firmware.ps1 -Acao ota -Porta "$(PORT)" -Jobs "$(JOBS)"
