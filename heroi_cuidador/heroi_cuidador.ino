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
//bool segundo_exercicio = false;
//bool terceiro_exercicio = false;
//bool finalizou_exercicio = false;

int valor_emg_atual;
int valores_emg[20] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int media_emg;

float angulo_x;
float angulo_y;
float angulo_z;

int contador_postura_correta;
int contador_postura_incorreta;
int valor_calibracao;

//Constantes
//const int  TETO_VOLTAGEM_REPOUSO = 600;
//const int  PISO_VOLTAGEM_FADIGA = 800;

const int CONTADOR_EMG = 40;
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
//    Serial.println("{" + (String) "MSG" + (String) "Entrou em iniciar" + (String) "}");
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
  if (media_emg == CONTADOR_EMG) {
    musculo_relaxado = false;

    //Disparar motor
//    digitalWrite(MOTOR_ESQUERDO, HIGH);
   // Serial.println("Começou Fadiga Muscular");
    delay(2000);
  }
}

//Musculo em fadiga
void checaTerminouFadigaMuscular() {
  
  //Fadiga terminou
  if (media_emg == 0) {
    musculo_relaxado_2 = true;
  }
}

//Musculo saiu da situação de fadiga
void resetaSistemaEMG() {
  //Desliga motor
  digitalWrite(MOTOR_ESQUERDO, LOW);
 // Serial.println("Terminou Fadiga Muscular");

  media_emg = 0;

  musculo_relaxado_2 = false;
  musculo_relaxado = true;
}

void resetaSistemaGiro() {
  // Desliga motor
  digitalWrite(MOTOR_ESQUERDO, LOW);
  //Serial.println("{" + (String) "MSG" + (String) "Entrou em modo_operacao" + (String) "}");

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
  }
}

void checaPosturaCorreta() {
  if (angulo_x > ANGULO_POSTURA_CORRETA_X - VARIACAO_INFERIOR_X && angulo_x < ANGULO_POSTURA_CORRETA_X + VARIACAO_SUPERIOR_X) {
    contador_postura_correta = contador_postura_correta + 1;
  }

  if (contador_postura_correta == MAX_CORRETA) {
    postura_ereta_2 = true;
  }
}

void leitura() {
  mpu6050.update();
  angulo_x = mpu6050.getAngleX();
  angulo_y = mpu6050.getAngleY();
  angulo_z = mpu6050.getAngleZ();

//  Serial.println("{" + (String) "MPU" + (String) angulo_x + " " + (String) angulo_y + " " + (String) angulo_z + "}");

//  if(calibracao) {
//    Serial.println("{" + (String) "CAL"+ (String) angulo_x + "}");
//  }
    
}

void atualizaValores() {

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

//  Serial.println("{"+ (String) "EMG" + (String) media_emg + "}");
}

bool modo_operacao() {
  //dia a dia
  Serial.println("Entrou no modo de operação");
  while (operacao) {
    String comando = "";

    leitura();
    atualizaValores();

    if (postura_ereta) {
      checaPostura();
    }

    if (musculo_relaxado) {
      identificaFadigaMuscular();
    }

    if (!musculo_relaxado && !postura_ereta) {
        checaTerminouFadigaMuscular();
        checaPosturaCorreta();

        if (parada_aux) {
           Serial.println("Movimento incorreto");
         // Serial.println("{" + (String) "MOV" + (String) "incorreto" + (String) "}");
          digitalWrite(MOTOR_ESQUERDO, HIGH);
          parada_aux = false;
          delay(2000);
        }

        if (musculo_relaxado_2 && postura_ereta_2) {
          digitalWrite(MOTOR_ESQUERDO, LOW);
          resetaSistemaEMG();
          resetaSistemaGiro();
          parada_aux = true;
         // Serial.println("{" + (String) "MOV" + (String) "correto" + (String) "}");
        Serial.println("Movimento correto");
        }
    }

    comando = receber_bluetooth();
    if (comando == "calibracao") {
      operacao = false;
      calibracao = true;
    }

    if (comando == "resetar") {
        resetaSistemaEMG();
        resetaSistemaGiro();
        parada_aux = true;
        //Serial.println("{" + (String) "MOV" + (String) "correto" + (String) "}");
        Serial.println("Movimento correto");
    }
  }

  delay(20);

  return true;
}

bool iniciar_calibracao() {
  Serial.println("Entrou no modo de Calibração");
  String comando = "";
  
  while (calibracao) {
    //checar envio para o celular
    comando = receber_bluetooth();
    if (comando == "operacao") {
      calibracao = false;
      operacao = true;
    }
    if(comando == "iniciar_calibracao"){
      while ((comando != "proxima_calibracao")&&(comando!=" operacao")) {
        //ler mpu
        leitura();
        if(angulo_x<=90&&angulo_x>=0){
          if(90-angulo_x>valor_calibracao){
           valor_calibracao=90-angulo_x;
          } 
         }
         
        
       comando = receber_bluetooth();
      }
    }
     if (comando == "proxima_calibracao") {
      Serial.println( (String) "CAL" + (String) valor_calibracao);
      //enviar valor máximo pro app
      valor_calibracao=0;
    }
    if (comando == "fim_calibracao") {
      calibracao = false;
      operacao = true;
    }
     if (comando.substring(0, 3) == "CAL") {
     // comando.substring(3).toFloat();
       Serial.println("Valor calibrado:"+(String)comando.substring(3) );
       delay(5000);
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
