// Inclure la bibliothèque painlessMesh
#include <painlessMesh.h>

// Inclure la bibliothèque ArduinoJson
#include <ArduinoJson.h>

// Définir le nom du réseau maillé et le mot de passe
#define   MESH_PREFIX     "ESP32-Mesh"
#define   MESH_PASSWORD   "12345678"

// Créer un objet painlessMesh
painlessMesh  mesh;

// Définir les identifiants des 5 modules ESP32
#define   NODE_1_ID       1234567890
#define   NODE_2_ID       2345678901
#define   NODE_3_ID       3456789012
#define   NODE_4_ID       4567890123
#define   NODE_5_ID       5678901234

// Définir le type de chaque module (MESH_ROOT pour le nœud principal, MESH_IDLE pour les nœuds secondaires)
#define   NODE_1_TYPE     MESH_ROOT // Changer la valeur de retour en MESH_ROOT pour le module principal
#define   NODE_2_TYPE     MESH_IDLE // Changer la valeur de retour en MESH_IDLE pour les modules secondaires
#define   NODE_3_TYPE     MESH_IDLE
#define   NODE_4_TYPE     MESH_IDLE
#define   NODE_5_TYPE     MESH_IDLE

// Définir une fonction qui renvoie l'identifiant du module courant
uint32_t getNodeId() {
  return mesh.getNodeId();
}

// Définir une fonction qui renvoie le type du module courant en fonction de son identifiant
mesh_type_t getNodeType(uint32_t nodeId) {
  switch (nodeId) {
    case NODE_1_ID: return NODE_1_TYPE;
    case NODE_2_ID: return NODE_2_TYPE;
    case NODE_3_ID: return NODE_3_TYPE;
    case NODE_4_ID: return NODE_4_TYPE;
    case NODE_5_ID: return NODE_5_TYPE;
    default: return MESH_IDLE;
  }
}

// Définir une fonction qui s'exécute lorsque le module se connecte au réseau maillé
void connectedCb() {
  Serial.printf("Le module %u s'est connecté au réseau maillé\n", getNodeId());
}

// Définir une fonction qui s'exécute lorsque le module reçoit un message d'un autre module
void receivedCb(uint32_t from, String &msg) {
  Serial.printf("Le module %u a reçu un message de %u : %s\n", getNodeId(), from, msg.c_str());
  
  // Créer un objet DynamicJsonDocument pour stocker les données JSON reçues
  DynamicJsonDocument doc(4096);

  // Décoder la chaîne JSON reçue et la stocker dans l'objet doc
  deserializeJson(doc, msg);

  // Vérifier si le message contient un champ "tableau"
  if (doc.containsKey("tableau")) {
    // Extraire le tableau JSON du champ "tableau"
    JsonArray array = doc["tableau"];

    // Parcourir le tableau JSON et le copier dans le tableau local ou dans le tableau global selon le type du nœud courant
    for (int i = 0; i < 10; i++) {
      for (int j = 0; j < 10; j++) {
        for (int k = 0; k < 40; k++) {
          // Copier la valeur du tableau JSON à la position [i][j][k] dans le tableau local ou dans le tableau global à la même position
          if (getNodeType(getNodeId()) == MESH_ROOT) {
            // Si le nœud courant est le nœud principal, copier la valeur dans le tableau global à la position [from - 1][i][j][k]
            // Le from - 1 correspond au numéro du nœud secondaire qui a envoyé les données (par exemple, si from = NODE_2_ID, alors from - 1 = 1)
            tableau_global[from - 1][i][j][k] = array[i][j][k];
          } else {
            // Si le nœud courant est un nœud secondaire, copier la valeur dans le tableau local à la position [i][j][k]
            tableau_local[i][j][k] = array[i][j][k];
          }
        }
      }
    }

    // Afficher un message pour indiquer que le tableau local ou global a été mis à jour
    if (getNodeType(getNodeId()) == MESH_ROOT) {
      // Si le nœud courant est le nœud principal, afficher un message pour indiquer que le tableau global a été mis à jour
      Serial.printf("Le module %u a mis à jour son tableau global avec les données du module %u\n", getNodeId(), from);
    } else {
      // Si le nœud courant est un nœud secondaire, afficher un message pour indiquer que le tableau local a été mis à jour
      Serial.printf("Le module %u a mis à jour son tableau local avec les données du module %u\n", getNodeId(), from);
    }
  }

  // Vérifier si le message contient un champ "demande"
  if (doc.containsKey("demande")) {
    // Extraire la valeur du champ "demande"
    String demande = doc["demande"];

    // Vérifier si la demande est une demande de mise à jour du tableau
    if (demande == "mise à jour") {
      // Appeler la fonction qui convertit le tableau local en une chaîne JSON et qui l'envoie au nœud demandeur
      sendTableau(from);
    }
  }
}

// Définir une variable globale pour stocker le tableau 3 dimensions de 10 par 10 par 40 sur les nœuds secondaires
uint8_t tableau_local[10][10][40];

// Définir une variable globale pour stocker le tableau 4 dimensions de 5 par 10 par 10 par 40 sur le nœud principal
uint8_t tableau_global[5][10][10][40];

// Définir une fonction qui convertit le tableau local en une chaîne JSON et qui l'envoie au nœud destinataire
void sendTableau(uint32_t to) {
  
   // Créer un objet DynamicJsonDocument pour stocker les données JSON à envoyer
   DynamicJsonDocument doc(4096);

   // Ajouter un champ "tableau" à l'objet doc
   JsonArray array = doc.createNestedArray("tableau");

   // Parcourir le tableau local et le copier dans le tableau JSON
   for (int i = 0; i < 10; i++) {
     JsonArray array_i = array.createNestedArray();
     for (int j = 0; j < 10; j++) {
       JsonArray array_j = array_i.createNestedArray();
       for (int k = 0; k < 40; k++) {
         // Copier la valeur du tableau local à la position [i][j][k] dans le tableau JSON à la même position
         array_j.add(tableau_local[i][j][k]);
       }
     }
   }

   // Convertir l'objet doc en une chaîne JSON
   String msg;
   serializeJson(doc, msg);

   // Envoyer la chaîne JSON au nœud destinataire
   mesh.sendSingle(to, msg);

   // Afficher un message pour indiquer que le tableau local a été envoyé
   Serial.printf("Le module %u a envoyé son tableau local au module %u\n", getNodeId(), to);
}

// Définir une fonction qui envoie une demande de mise à jour du tableau au nœud principal
void requestTableau() {

  // Créer un objet DynamicJsonDocument pour stocker les données JSON à envoyer
  DynamicJsonDocument doc(4096);

  // Ajouter un champ "demande" à l'objet doc avec la valeur "mise à jour"
  doc["demande"] = "mise à jour";

  // Convertir l'objet doc en une chaîne JSON
  String msg;
  serializeJson(doc, msg);

  // Envoyer la chaîne JSON au nœud principal
  mesh.sendSingle(NODE_1_ID, msg);

  // Afficher un message pour indiquer que la demande de mise à jour a été envoyée
  Serial.printf("Le module %u a envoyé une demande de mise à jour du tableau au module %u\n", getNodeId(), NODE_1_ID);
}


void setup() {
  // Initialiser la communication série à 115200 bauds
  Serial.begin(115200);

  // Configurer le réseau maillé avec le nom et le mot de passe définis plus haut
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION); 
  mesh.init(MESH_PREFIX, MESH_PASSWORD);

  // Configurer le type et le nœud parent du module courant en fonction de son identifiant
  mesh.setRoot(getNodeType(getNodeId()) == MESH_ROOT);
  mesh.setContainsRoot(getNodeType(getNodeId()) == MESH_ROOT);
  
  // Assigner le nœud principal comme nœud parent pour tous les nœuds secondaires
  if (getNodeType(getNodeId()) == MESH_IDLE) {
    mesh.setStaticIP(NODE_1_ID);
  }

  // Enregistrer les fonctions à exécuter lors des événements du réseau maillé
  mesh.onConnected(connectedCb);
  mesh.onReceive(receivedCb);

  // Initialiser le tableau local avec des valeurs aléatoires entre 0 et 255
  randomSeed(analogRead(0));
  for (int i = 0; i < 10; i++) {
    for (int j = 0; j < 10; j++) {
      for (int k = 0; k < 40; k++) {
        tableau_local[i][j][k] = random(0,256);
      }
    }
  }

}

void loop() {
  // Mettre à jour l'état du réseau maillé
  mesh.update();

  // Si le nœud courant est un nœud secondaire, envoyer périodiquement son tableau local au nœud principal
  if (getNodeType(getNodeId()) == MESH_IDLE) {
    // Définir un intervalle de temps entre chaque envoi (par exemple, toutes les 10 secondes)
    static unsigned long lastTime = millis();
    unsigned long now = millis();
    unsigned long interval = 10000;

    // Vérifier si l'intervalle de temps est écoulé
    if (now - lastTime > interval) {
      // Envoyer le tableau local au nœud principal
      sendTableau(NODE_1_ID);

      // Réinitialiser le compteur de temps
      lastTime = now;
    }
    
    // Si un bouton est appuyé, envoyer une demande de mise à jour du tableau au nœud principal
    // Définir le numéro de la broche où le bouton est connecté (par exemple, la broche D2)
    int buttonPin = D2;

    // Lire l'état du bouton
    int buttonState = digitalRead(buttonPin);

    // Vérifier si le bouton est appuyé
    if (buttonState == HIGH) {
      // Envoyer une demande de mise à jour du tableau au nœud principal
      requestTableau();
    }
    
  }

}
