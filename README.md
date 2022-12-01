# Instruções para conectar o sensor à rede Wi-Fi:

**Por padrão, os sensores estão configurados para conectarem-se à uma rede com SSID = *Sensores* e Senha = *MEGA12345***

Para mudar essas configurações, basta mudar os valores de ***ssid*** e ***password*** no arquivo ***wificonfig.h*** e fazer o upload na placa novamente.

Link para configurar o Arduino IDE para ESP8266: https://www.filipeflop.com/blog/programar-nodemcu-com-ide-arduino/

Para fazer o upload, são necessários 3 arquivos: ***zbx-esp-env.ino***, ***wificonfig.h*** e ***sensors.h***.

As bibliotecas necessárias estão na pasta ***libraries***. Basta acrescentá-las na pasta de bibliotecas.

**IMPORTANTE: Quando for fazer o upload do código nas ESPs, tem que desconectar o fio que liga o pino *D0* ao pino *RST*. Após fazer o upload, conecte novamente o fio antes de ligar o sensor**

O endereço do sensor usado para conectar ao zabbix é printado no monitor serial assim que o sensor é ligado e se conecta à rede.

# Instruções para conectar o sensor ao zabbix:

1. Importe o template "***Template Module ICMP Ping***" para o zabbix
2. Importe o template "***Template_ZBX-ESP-ENV***" para o zabbix
3. Crie um Host com o Nome que desejar e Associe ao "***Template_ZBX-ESP-ENV***"
4. Associe um grupo de Hosts
5. Em Interfaces, adicione uma do tipo "***IPMI***" com o endereço do sensor na rede local

![alt text](https://github.com/shirasagihimegimi/Tupi/blob/main/imagens/host.png?raw=true)

6. No host recem criado, clique em "***descoberta***"

![alt text](https://github.com/shirasagihimegimi/Tupi/blob/main/imagens/descoberta.png?raw=true)

7. Selecione o Host recem criado e clique em "***Ativar***"
8. Selecione o Host recem criado e clique em "***Execute Now***" 

***IMPORTANTE: O passo 8. deve ser executado no intervalo em que o sensor está ligado. 
O sensor ficará ligado nos primeiros 30 segundos após conectar a pilha (não deu tempo de arrumar isso, desculpa).***

9. Pronto. Se o zabbix conseguiu realizar a descoberta dos itens no endereço do sensor, os dados de temperatura e umidade já estarão disponíveis.

# Instruções para conexões de hardware:
Caso os fios se desconectem.
## Sensor de Temperatura/Humidade:
- SDA -> D2
- SCL -> D1
- GND -> G 
- 3V -> 3V
## Bateria:
- Positivo -> VIN
- Negativo -> G
## ESP (Para o funcionamento da função deepsleep):
- D0 -> RST 
