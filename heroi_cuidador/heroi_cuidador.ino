#include <MPU6050_tockn.h>
#include<SoftwareSerial.h>

SoftwareSerial bluetooth(10, 11);
MPU6050 mpu6050(Wire);

bool musculo_relaxado;
bool postura_ereta;

bool musculo_relaxado_2;
bool postura_ereta_2;

bool parada_aux;

bool aux;
bool aux2;

bool calibracao = false;
bool operacao = false;

int valor_emg_atual;
int media_emg;
int j;

float angulo_x;
float angulo_y;
float angulo_z;

int contador_postura_correta;
int contador_postura_incorreta;
int valor_calibracao;

//Constantes
//const int  TETO_VOLTAGEM_REPOUSO = 600;
//const int  PISO_VOLTAGEM_FADIGA = 800;

const int CONTADOR_EMG = 30;
const int FAIXA_EMG_ATIVACAO = 800;

const float ANGULO_POSTURA_CORRETA_X = 90.0;
const float VARIACAO_INFERIOR_X = 65.0;
const float VARIACAO_SUPERIOR_X = 20.0;
const int  MAX_CORRETA = 20;
const int  MAX_INCORRETA = 40;

const int MOTOR_ESQUERDO = 11;
const int MOTOR_DIREITO = 10;


void setup() {
  Serial.begin(9600);
  bluetooth.begin(9600);

  pinMode(MOTOR_ESQUERDO, OUTPUT);
  pinMode(MOTOR_DIREITO, OUTPUT);

  digitalWrite(MOTOR_ESQUERDO, LOW);
  digitalWrite(MOTOR_DIREITO, LOW);
  
  Wire.begin();
  mpu6050.begin();
  mpu6050.setGyroOffsets(-1.20, -4.25, 1.53);
  valor_calibracao=0;
  musculo_relaxado = true;
  postura_ereta = true;

  musculo_relaxado_2 = true;
  postura_ereta_2 = true;

  parada_aux = true;
  
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
    aux = iniciar_calibracao();
  }
  else if (modo == "operacao" || aux ) {
    calibracao = false;
    operacao = true;
    aux = false;
    aux2 = modo_operacao();
  }
  
  delay(20);
}

void identificaFadigaMuscular() {
  if (media_emg == CONTADOR_EMG) {
    musculo_relaxado = false;

    //Disparar motor
//    digitalWrite(MOTOR_ESQUERDO, HIGH);
    Serial.println("{MSGComeçou Fadiga Muscular}");
  }
}

//Musculo em fadiga
bool checaTerminouFadigaMuscular() {
  //Fadiga terminou
  if (media_emg == 0) {
    Serial.println("{MSGTerminou Fadiga Muscular}");
    musculo_relaxado_2 = true;
    return true;
  }

  return false;
}

//Musculo saiu da situação de fadiga
void resetaSistemaEMG() {
  //Desliga motor
  digitalWrite(MOTOR_ESQUERDO, LOW);
  Serial.println("{MSGTerminou Fadiga Muscular}");

  media_emg = 0;

  musculo_relaxado_2 = false;
  musculo_relaxado = true;
}

void resetaSistemaGiro() {
  // Desliga motor
  digitalWrite(MOTOR_ESQUERDO, LOW);
  Serial.println("{" + (String) "MSG" + (String) "Postura Correta" + (String) "}");

  contador_postura_correta = 0;
  contador_postura_incorreta = 0;

  postura_ereta_2 = false;
  postura_ereta = true;
}

//Checa se a postura está incorreta
void checaPostura() {
  if (angulo_x < ANGULO_POSTURA_CORRETA_X - VARIACAO_INFERIOR_X || angulo_x > ANGULO_POSTURA_CORRETA_X + VARIACAO_SUPERIOR_X) {
    contador_postura_incorreta = contador_postura_incorreta + 1;
  }

  if (contador_postura_incorreta == MAX_INCORRETA) {
    postura_ereta = false;
    contador_postura_incorreta = 0;
     Serial.println("{" + (String) "MSG" + (String) "Postura errada" + (String) "}");
  }
}

bool checaPosturaCorreta() {
  if (angulo_x > ANGULO_POSTURA_CORRETA_X - VARIACAO_INFERIOR_X && angulo_x < ANGULO_POSTURA_CORRETA_X + VARIACAO_SUPERIOR_X) {
    contador_postura_correta = contador_postura_correta + 1;
  }

  if (contador_postura_correta == MAX_CORRETA) {
    Serial.println("{" + (String) "MSG" + (String) "Postura Correta" + (String) "}");
    contador_postura_correta = 0;
    postura_ereta_2 = true;
    return true;
  }

  return false;
}

void leitura_mpu() {
  mpu6050.update();
  angulo_x = mpu6050.getAngleX();
  angulo_y = mpu6050.getAngleY();
  angulo_z = mpu6050.getAngleZ();

}

void leitura_emg() {

  //Leitura do valor do sensor EMG
  valor_emg_atual = analogRead(A0);
  
  if(valor_emg_atual > FAIXA_EMG_ATIVACAO) {
    if(media_emg < CONTADOR_EMG ) {
      media_emg = media_emg + 1;
    }
  }

  if(valor_emg_atual < FAIXA_EMG_ATIVACAO) {
    if(media_emg > 0 ) {
      media_emg = media_emg - 1;
    }
  }
}

bool modo_operacao() {
  //dia a dia
  Serial.println("{MSGEntrou em modo_operacao}");
  while (operacao) {
    String comando = "";

    //faz a leitura dos dados do giroscópio 
    leitura_mpu();
    //faz a leitura dos dados do emg
    leitura_emg();

    if (postura_ereta) {
      //confere os dados lidos no mpu
      checaPostura();
    }

    if (musculo_relaxado) {
      //confere os dados lidos no emg
      identificaFadigaMuscular();
    }

    if (!postura_ereta && musculo_relaxado) {
      //não deve ativar o motor só a postura está errada
      bool corrigiu = checaPosturaCorreta();

      if (corrigiu) {
        postura_ereta = true;
        postura_ereta_2 = false;
      }
    }

    if (postura_ereta && !musculo_relaxado) {
      //não deve ativar o motor
      bool corrigiu = checaTerminouFadigaMuscular();

      if (corrigiu) {
        musculo_relaxado = true;
        musculo_relaxado_2 = false;
      }
    }
    
    if (!musculo_relaxado && !postura_ereta) {
      //deve ativar o motor
        if (parada_aux) {
          parada_aux = false;
          digitalWrite(MOTOR_ESQUERDO, HIGH);
          Serial.println("{MOVincorreto}");
          delay(3000);
          //usamos os 3 segundos para mostrar melhor aos cuidadores o movimento incorreto
        }

        checaTerminouFadigaMuscular();
        checaPosturaCorreta();

        if (musculo_relaxado_2 && postura_ereta_2) {
          digitalWrite(MOTOR_ESQUERDO, LOW);
          resetaSistemaEMG();
          resetaSistemaGiro();
          parada_aux = true;
          Serial.println("{MOVanalise}");
        }
    }

    comando = receber_bluetooth();
    if (comando == "calibracao") {
      operacao = false;
      calibracao = true;
    }
    else if (comando == "resetar") {
        resetaSistemaEMG();
        resetaSistemaGiro();
        parada_aux = true;
        Serial.println("{MOVanalise}");
    }
    else if (comando == "resultado") {
      operacao = false;
      calibracao = false;
      aux = false;
      aux2 = false;
      return false;
    }
  }

  delay(20);

  return true;
}

bool iniciar_calibracao() {
  Serial.println("{MSGEntrou no modo de Calibracao}");
  String comando = "";
  
  while (calibracao) {
    
    //checar envio para o celular
    comando = receber_bluetooth();
    
    if (comando == "operacao") {
      calibracao = false;
      operacao = true;
    }
    else if (comando == "iniciar_exercicio") {
        
        String comando_exercicio = "";
        valor_calibracao = 0.0;
        
        while(comando_exercicio != "finalizar_exercicio") {
          //ler mpu
          leitura_mpu();

          float variacao = 90 - angulo_x;
        
          if(variacao > valor_calibracao) {
            if(variacao >= 0 && variacao <= 90) {
              valor_calibracao = variacao;
            }
          }

          comando_exercicio = receber_bluetooth();
          delay(20);
        }

        if (comando_exercicio == "finalizar_exercicio") {
          Serial.println("{" + (String) "CAL" + (String) valor_calibracao + (String) "}");
          //enviar valor máximo pro app
        }
    }
    else if (comando == "fim_calibracao") {
      calibracao = false;
      operacao = true;
    }

    delay(20);
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
