#include <MPU6050_tockn.h>
#include<SoftwareSerial.h>

SoftwareSerial bluetooth(10, 11);
MPU6050 mpu6050(Wire);

bool musculo_relaxado;
bool postura_ereta;

bool aux;
bool aux2;

bool calibracao = false;
bool operacao = false;
//bool segundo_exercicio = false;
//bool terceiro_exercicio = false;
//bool finalizou_exercicio = false;

int valor_emg_atual;
int valores_emg[5] = {0, 0, 0, 0, 0};
int media_emg;

float angulo_x;
float angulo_y;
float angulo_z;

int contador_postura_correta;
int contador_postura_incorreta;

//Constantes
const int  TETO_VOLTAGEM_REPOUSO = 600;
const int  PISO_VOLTAGEM_FADIGA = 800;

const float ANGULO_POSTURA_CORRETA = 90.0;
const float VARIACAO_INFERIOR = 20.0;
const float VARIACAO_SUPERIOR = 20.0;
const int  MAX_CORRETA = 20;
const int  MAX_INCORRETA = 20;

void setup() {
  Serial.begin(9600);
  bluetooth.begin(9600);

  calibracao = false;
  operacao = false;
  
  aux2 = false;
  aux = false;
}

void loop() {
  String  modo = receber_bluetooth();
  
  if (modo == "calibracao" || aux2) {
    calibracao = true;
    operacao = false;
    aux2 = false;
    Serial.println("{" + (String) "MSG" + (String) "Entrou em iniciar" + (String) "}");
    aux = iniciar_calibracao();
  }
  if (modo == "operacao" || aux ) {
    calibracao = false;
    operacao = true;
    aux = false;
    aux2 = modo_operacao();
  }
  
  delay(20);
}

void identificaFadigaMuscular() {
  if (media_emg > PISO_VOLTAGEM_FADIGA) {
    musculo_relaxado = false;
  }
}

//Musculo em fadiga
void checaTerminouFadigaMuscular() {
  //Disparar motor

  //Fadiga terminou
  if (media_emg < TETO_VOLTAGEM_REPOUSO) {
    musculo_relaxado = true;
  }
}

//Musculo saiu da situação de fadiga
void resetaSistemaEMG() {
  //Desliga motor

}
void resetaSistemaGiro() {
  // Desliga motor

  contador_postura_correta = 0;
  contador_postura_incorreta = 0;
}

//Checa se a postura está incorreta
void checaPostura() {
  if (angulo_x < ANGULO_POSTURA_CORRETA - VARIACAO_INFERIOR || angulo_x > ANGULO_POSTURA_CORRETA + VARIACAO_SUPERIOR) {
    contador_postura_incorreta = contador_postura_incorreta + 1;
  }

  if (contador_postura_incorreta == MAX_INCORRETA) {
    postura_ereta = false;
  }
}

void checaPosturaCorreta() {
  if (angulo_x > ANGULO_POSTURA_CORRETA - VARIACAO_INFERIOR && angulo_x < ANGULO_POSTURA_CORRETA + VARIACAO_SUPERIOR) {
    contador_postura_correta = contador_postura_correta + 1;
  }

  if (contador_postura_correta == MAX_CORRETA) {
    postura_ereta = true;
  }
}

void leitura() {
  //  mpu6050.update();
  //  angulo_x = mpu6050.getAngleX();
  //  angulo_y = mpu6050.getAngleY();
  //  angulo_z = mpu6050.getAngleZ();
  angulo_x = 10;
  angulo_y = 20;
  angulo_z = 30;
  Serial.println("{" + (String) "MPU" + (String) angulo_x + " " + (String) angulo_y + " " + (String) angulo_z + "}");
}

void atualizaValores() {

  //Reinicia o valor da média
  media_emg = 0;

  //Leitura do valor do sensor EMG
  valor_emg_atual = analogRead(A0);

  for (int i = 0; i < (5 - 1); i++) {
    valores_emg[i] = valores_emg[i + 1];
    media_emg += valores_emg[i];
  }

  valores_emg[4] = valor_emg_atual;
  media_emg += valor_emg_atual;

  //Calcula a média dos valores do vetor
  media_emg = media_emg / 5;

  Serial.println("{"+ (String) "EMG" + (String) media_emg + "}");
}

bool modo_operacao() {
  //dia a dia
  Serial.println("{" + (String) "MSG" + (String) "Entrou em modo_operacao" + (String) "}");
  while (operacao) {
    String comando = "";

    leitura();
    atualizaValores();

    if (musculo_relaxado && postura_ereta) {
      identificaFadigaMuscular();
      checaPostura();
    }

    if (!musculo_relaxado || !postura_ereta) {
      if (!musculo_relaxado) {
        checaTerminouFadigaMuscular();

        if (musculo_relaxado) {
          resetaSistemaEMG();
        }
      }
      if (!postura_ereta) {
        checaPosturaCorreta();

        if (postura_ereta) {
          resetaSistemaGiro();
        }
      }
    }

    comando = receber_bluetooth();
    if (comando == "calibracao") {
      operacao = false;
      calibracao = true;
    }
  }

  return true;
}

bool iniciar_calibracao() {
  Serial.println("{" + (String) "MSG" + (String) "Entrou em iniciar_calibracao" + (String) "}");
  String comando = "";
  
  while (calibracao) {
    leitura();
    atualizaValores();
    
    //checar envio para o celular
    comando = receber_bluetooth();
    if (comando == "operacao") {
      calibracao = false;
      operacao = true;
    }
  }

  return true;
}


String receber_bluetooth() {
  String  comando = "";
  if (Serial.available()) {
    while (Serial.available()) {
      char caracter = Serial.read();
      comando += caracter;
      delay(10);
    }

  }
  return comando;
}
