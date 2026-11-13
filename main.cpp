#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <stdexcept>

// =====================================================================
// 1. ÉNUMÉRATIONS (MACHINES À ÉTATS DU DIAGRAMME UML)
// =====================================================================
enum class StatutReservation {
    EN_ATTENTE,
    CONFIRMEE,
    ANNULEE
};

enum class StatutRemboursement {
    NON_APPLICABLE,
    REMBOURSE_INTEGRAL,
    REMBOURSE_PARTIEL
};

std::string statutResToString(StatutReservation s) {
    switch (s) {
        case StatutReservation::EN_ATTENTE: return "EN_ATTENTE";
        case StatutReservation::CONFIRMEE:  return "CONFIRMEE";
        case StatutReservation::ANNULEE:    return "ANNULEE";
    }
    return "INCONNU";
}

std::string statutRembToString(StatutRemboursement s) {
    switch (s) {
        case StatutRemboursement::NON_APPLICABLE:     return "NON_APPLICABLE";
        case StatutRemboursement::REMBOURSE_INTEGRAL: return "REMBOURSE_INTEGRAL";
        case StatutRemboursement::REMBOURSE_PARTIEL:  return "REMBOURSE_PARTIEL";
    }
    return "INCONNU";
}

// Déclarations anticipées
class Reservation;
class Paiement;

// =====================================================================
// 2. CLASSES DU MODÈLE UML
// =====================================================================
class Passager {
private:
    int id;
    std::string nom;
    std::string prenom;
    std::string email;
    std::string numeroPasseport;

public:
    Passager(int id, std::string nom, std::string prenom, std::string email, std::string passeport)
        : id(id), nom(std::move(nom)), prenom(std::move(prenom)),
          email(std::move(email)), numeroPasseport(std::move(passeport)) {}

    int getId() const { return id; }
    std::string getNomComplet() const { return nom + " " + prenom; }
    std::string getEmail() const { return email; }
    std::string getPasseport() const { return numeroPasseport; }
};

class Vol {
private:
    int id;
    std::string numeroVol;
    std::string origine;
    std::string destination;
    std::time_t dateDepart; // Timestamp Unix
    int capaciteTotale;
    double prixBillet;
    std::vector<std::shared_ptr<Reservation>> reservations;

public:
    Vol(int id, std::string num, std::string orig, std::string dest,
        std::time_t depart, int capacite, double prix)
        : id(id), numeroVol(std::move(num)), origine(std::move(orig)),
          destination(std::move(dest)), dateDepart(depart),
          capaciteTotale(capacite), prixBillet(prix) {}

    int getId() const { return id; }
    std::string getNumeroVol() const { return numeroVol; }
    std::string getTrajet() const { return origine + " -> " + destination; }
    std::time_t getDateDepart() const { return dateDepart; }
    int getCapaciteTotale() const { return capaciteTotale; }
    double getPrixBillet() const { return prixBillet; }

    void ajouterReservation(const std::shared_ptr<Reservation>& res) {
        reservations.push_back(res);
    }

    int calculerPlacesConfirmees() const;
    int calculerPlacesDisponibles() const {
        return capaciteTotale - calculerPlacesConfirmees();
    }
    bool verifierDisponibilite() const {
        return calculerPlacesDisponibles() > 0;
    }
};

class Paiement {
private:
    int id;
    double montant;
    std::time_t datePaiement;
    std::string methode;

public:
    Paiement(int id, double montant, std::string methode)
        : id(id), montant(montant), datePaiement(std::time(nullptr)), methode(std::move(methode)) {}

    int getId() const { return id; }
    double getMontant() const { return montant; }
    std::string getMethode() const { return methode; }
};

class Reservation {
private:
    int id;
    std::shared_ptr<Passager> passager; // Association 1..1
    std::shared_ptr<Vol> vol;           // Association 1..1
    std::time_t dateReservation;
    StatutReservation statut;
    StatutRemboursement statutRemboursement;
    std::shared_ptr<Paiement> paiement; // Association 0..1

public:
    Reservation(int id, std::shared_ptr<Passager> p, std::shared_ptr<Vol> v)
        : id(id), passager(std::move(p)), vol(std::move(v)),
          dateReservation(std::time(nullptr)),
          statut(StatutReservation::EN_ATTENTE),
          statutRemboursement(StatutRemboursement::NON_APPLICABLE),
          paiement(nullptr) {}

    int getId() const { return id; }
    std::shared_ptr<Passager> getPassager() const { return passager; }
    std::shared_ptr<Vol> getVol() const { return vol; }
    StatutReservation getStatut() const { return statut; }
    StatutRemboursement getStatutRemboursement() const { return statutRemboursement; }
    std::shared_ptr<Paiement> getPaiement() const { return paiement; }

    // Validation suite au paiement
    void valider(int paiementId, const std::string& methode) {
        if (statut == StatutReservation::ANNULEE) {
            throw std::runtime_error("RG-05 : Impossible de payer une reservation annulee.");
        }
        if (statut == StatutReservation::CONFIRMEE) {
            throw std::runtime_error("Cette reservation est deja payee et confirmee.");
        }
        if (!vol->verifierDisponibilite()) {
            throw std::runtime_error("RG-02/RG-06 : Plus aucune place disponible sur ce vol.");
        }

        paiement = std::make_shared<Paiement>(paiementId, vol->getPrixBillet(), methode);
        statut = StatutReservation::CONFIRMEE;
    }

    // Annulation et calcul du remboursement
    void annuler() {
        if (statut == StatutReservation::ANNULEE) {
            throw std::runtime_error("Cette reservation est deja annulee.");
        }

        if (statut == StatutReservation::CONFIRMEE) {
            // RG-07 : Remboursement selon delai avant depart (24h = 86400s)
            double secondesAvantDepart = std::difftime(vol->getDateDepart(), std::time(nullptr));
            if (secondesAvantDepart > 86400.0) {
                statutRemboursement = StatutRemboursement::REMBOURSE_INTEGRAL;
            } else {
                statutRemboursement = StatutRemboursement::REMBOURSE_PARTIEL;
            }
        } else {
            statutRemboursement = StatutRemboursement::NON_APPLICABLE;
        }

        statut = StatutReservation::ANNULEE;
    }
};

// Définition différée nécessitant la définition complète de Reservation
int Vol::calculerPlacesConfirmees() const {
    int count = 0;
    for (const auto& res : reservations) {
        if (res->getStatut() == StatutReservation::CONFIRMEE) {
            count++;
        }
    }
    return count;
}

// =====================================================================
// 3. GESTIONNAIRE CENTRAL (CONTRÔLEUR MÉTIER)
// =====================================================================
class SystemeGestionAerienne {
private:
    std::vector<std::shared_ptr<Passager>> passagers;
    std::vector<std::shared_ptr<Vol>> vols;
    std::vector<std::shared_ptr<Reservation>> reservations;

    int nextPassagerId = 1;
    int nextVolId = 1;
    int nextReservationId = 100;
    int nextPaiementId = 5000;

public:
    // --- PASSAGERS ---
    std::shared_ptr<Passager> enregistrerPassager(const std::string& nom, const std::string& prenom,
                                                  const std::string& email, const std::string& passeport) {
        for (const auto& p : passagers) {
            if (p->getEmail() == email || p->getPasseport() == passeport) {
                throw std::runtime_error("Un passager avec cet email ou passeport existe deja.");
            }
        }
        auto p = std::make_shared<Passager>(nextPassagerId++, nom, prenom, email, passeport);
        passagers.push_back(p);
        return p;
    }

    void listerPassagers() const {
        std::cout << "\n--- LISTE DES PASSAGERS ---\n";
        for (const auto& p : passagers) {
            std::cout << "[" << p->getId() << "] " << p->getNomComplet()
                      << " | Passeport: " << p->getPasseport()
                      << " | Email: " << p->getEmail() << "\n";
        }
    }

    // --- VOLS ---
    std::shared_ptr<Vol> creerVol(const std::string& numero, const std::string& orig,
                                  const std::string& dest, int heuresAvantDepart,
                                  int capacite, double prix) {
        for (const auto& v : vols) {
            if (v->getNumeroVol() == numero) {
                throw std::runtime_error("Ce numero de vol existe deja.");
            }
        }
        std::time_t depart = std::time(nullptr) + (heuresAvantDepart * 3600);
        auto v = std::make_shared<Vol>(nextVolId++, numero, orig, dest, depart, capacite, prix);
        vols.push_back(v);
        return v;
    }

    void listerVols() const {
        std::cout << "\n--- PROGRAMME DES VOLS ---\n";
        for (const auto& v : vols) {
            std::cout << "[" << v->getId() << "] Vol " << v->getNumeroVol()
                      << " (" << v->getTrajet() << ") | Prix: " << v->getPrixBillet() << " EUR"
                      << " | Places confirmees: " << v->calculerPlacesConfirmees()
                      << "/" << v->getCapaciteTotale() << "\n";
        }
    }

    // --- RÉSERVATIONS ---
    std::shared_ptr<Reservation> creerReservation(int passagerId, int volId) {
        // RG-01 : Passager doit être enregistré
        auto itP = std::find_if(passagers.begin(), passagers.end(),
                                [passagerId](const auto& p) { return p->getId() == passagerId; });
        if (itP == passagers.end()) {
            throw std::runtime_error("RG-01 : Passager introuvable. Enregistrement requis.");
        }

        auto itV = std::find_if(vols.begin(), vols.end(),
                                [volId](const auto& v) { return v->getId() == volId; });
        if (itV == vols.end()) {
            throw std::runtime_error("Vol introuvable.");
        }

        // RG-03 : Pas de double réservation active sur le même vol
        for (const auto& r : reservations) {
            if (r->getPassager()->getId() == passagerId &&
                r->getVol()->getId() == volId &&
                r->getStatut() != StatutReservation::ANNULEE) {
                throw std::runtime_error("RG-03 : Ce passager possede deja une reservation active pour ce vol.");
            }
        }

        // RG-02 : Capacité du vol
        if (!(*itV)->verifierDisponibilite()) {
            throw std::runtime_error("RG-02 : Capacite maximale atteinte pour ce vol.");
        }

        auto res = std::make_shared<Reservation>(nextReservationId++, *itP, *itV);
        reservations.push_back(res);
        (*itV)->ajouterReservation(res);
        return res;
    }

    void listerReservations() const {
        std::cout << "\n--- DOSSIERS DE RESERVATION ---\n";
        for (const auto& r : reservations) {
            std::cout << "Res #" << r->getId()
                      << " | Passager: " << r->getPassager()->getNomComplet()
                      << " | Vol: " << r->getVol()->getNumeroVol()
                      << " | Statut: " << statutResToString(r->getStatut())
                      << " | Remboursement: " << statutRembToString(r->getStatutRemboursement()) << "\n";
        }
    }

    // --- PAIEMENTS & ANNULATIONS ---
    void payerReservation(int reservationId, const std::string& methode) {
        auto itR = std::find_if(reservations.begin(), reservations.end(),
                                [reservationId](const auto& r) { return r->getId() == reservationId; });
        if (itR == reservations.end()) {
            throw std::runtime_error("Reservation introuvable.");
        }
        (*itR)->valider(nextPaiementId++, methode);
    }

    void annulerReservation(int reservationId) {
        auto itR = std::find_if(reservations.begin(), reservations.end(),
                                [reservationId](const auto& r) { return r->getId() == reservationId; });
        if (itR == reservations.end()) {
            throw std::runtime_error("Reservation introuvable.");
        }
        (*itR)->annuler();
    }
};

// =====================================================================
// 4. INTERFACE CONSOLE INTERACTIVE
// =====================================================================
int main() {
    SystemeGestionAerienne sys;

    // Données initiales
    sys.enregistrerPassager("Koffi", "Amina", "amina@mail.com", "BJ88412");
    sys.enregistrerPassager("Moreau", "Lucas", "lucas@mail.com", "FR99201");
    sys.creerVol("AF805", "Cotonou", "Paris", 48, 2, 620.0); // Depart dans 48h, capacite 2
    sys.creerVol("KP042", "Lome", "Abidjan", 12, 100, 185.0); // Depart dans 12h

    int choix = -1;
    while (choix != 0) {
        std::cout << "\n================ MENU AEROMANAGER (C++) ================\n"
                  << "1. Enregistrer un passager\n"
                  << "2. Creer un vol\n"
                  << "3. Creer une reservation\n"
                  << "4. Payer et valider une reservation\n"
                  << "5. Annuler une reservation\n"
                  << "6. Afficher l'etat global (Passagers, Vols, Reservations)\n"
                  << "0. Quitter\n"
                  << "Choix : ";
        if (!(std::cin >> choix)) break;

        try {
            if (choix == 1) {
                std::string nom, prenom, email, pass;
                std::cout << "Nom : "; std::cin >> nom;
                std::cout << "Prenom : "; std::cin >> prenom;
                std::cout << "Email : "; std::cin >> email;
                std::cout << "Passeport : "; std::cin >> pass;
                auto p = sys.enregistrerPassager(nom, prenom, email, pass);
                std::cout << "-> Passager enregistre avec ID : " << p->getId() << "\n";
            }
            else if (choix == 2) {
                std::string num, orig, dest;
                int heures, cap;
                double prix;
                std::cout << "Numero vol : "; std::cin >> num;
                std::cout << "Origine : "; std::cin >> orig;
                std::cout << "Destination : "; std::cin >> dest;
                std::cout << "Depart dans (heures) : "; std::cin >> heures;
                std::cout << "Capacite : "; std::cin >> cap;
                std::cout << "Prix (EUR) : "; std::cin >> prix;
                auto v = sys.creerVol(num, orig, dest, heures, cap, prix);
                std::cout << "-> Vol cree avec ID : " << v->getId() << "\n";
            }
            else if (choix == 3) {
                int pId, vId;
                sys.listerPassagers();
                sys.listerVols();
                std::cout << "ID Passager : "; std::cin >> pId;
                std::cout << "ID Vol : "; std::cin >> vId;
                auto r = sys.creerReservation(pId, vId);
                std::cout << "-> Reservation creee (ID #" << r->getId() << ") au statut EN_ATTENTE.\n";
            }
            else if (choix == 4) {
                int rId;
                std::string methode;
                sys.listerReservations();
                std::cout << "ID Reservation a payer : "; std::cin >> rId;
                std::cout << "Methode (CARTE / VIREMENT) : "; std::cin >> methode;
                sys.payerReservation(rId, methode);
                std::cout << "-> Paiement accepte. Place definitivement confirmee !\n";
            }
            else if (choix == 5) {
                int rId;
                sys.listerReservations();
                std::cout << "ID Reservation a annuler : "; std::cin >> rId;
                sys.annulerReservation(rId);
                std::cout << "-> Reservation annulee.\n";
            }
            else if (choix == 6) {
                sys.listerPassagers();
                sys.listerVols();
                sys.listerReservations();
            }
        } catch (const std::exception& e) {
            std::cerr << "\n[ERREUR METIER] " << e.what() << "\n";
        }
    }
    return 0;
}
