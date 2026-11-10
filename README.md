# bdd_relationnelle_simplifie

## Groupe : 
- Adam SIDHOUM
- Omar BENFDILA
- Arno Lumiron

## Sujet du projet : 

Coder une base de donnée relationnelle simplifiée

L'objectif de ce projet est de coder dans un langage de programmation compilé avec typage fort (C, Go, Rust, Haskell, etc.) un système de gestion de base de donnée simplifié.

Votre SGBD n'aura pas forcément besoin de gérer le SQL. Vous pourrez mettre à disposition une API simplifiée pour effectuer les opérations.

Votre SGBD devra permettre de :

Définir des tables dans un format simplifié
permettre de récupérer (requêter) des données
Gérer la notion d'index
Pour cela vous pourrez mettre à disposition des fonction comme :

create_table_struct(...)
insert_row(...)
select_values(...)
create_index(...)
Pour cela vous devrez réfléchir aux structure de données les plus pertinentes pour que votre base de données soit efficace.

Pour au moins un choix vous ferez une analyse de performance de deux implémentations de votre base de données : une vous avez choisis les structures de données que vous pensez optimale, et une ou vous avez choisi sciemment d'utiliser une structure de données sous optimale.

Vous devrez proposer plusieurs optimisation par rapport à une implémentation dite naive.

Vous devrez indiquer quelles sont les complexité des opérations typiques que vous proposez
