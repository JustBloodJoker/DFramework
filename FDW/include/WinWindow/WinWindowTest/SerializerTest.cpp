

#include "../WinWindow/Utils/Serializer/MacroReflection.h"
#include "../WinWindow/Utils/Serializer/BinarySerializer.h"


class Vec3 {
public:
    float x{}, y{}, z{};
    BEGIN_FIELD_REGISTRATION(Vec3)
        REGISTER_FIELD(x)
        REGISTER_FIELD(y)
        REGISTER_FIELD(z)
        END_FIELD_REGISTRATION()
};

class Weapon {
public:
    std::string name;
    int damage{};
    float weight{};
    BEGIN_FIELD_REGISTRATION(Weapon)
        REGISTER_FIELD(name)
        REGISTER_FIELD(damage)
        REGISTER_FIELD(weight)
        END_FIELD_REGISTRATION()
};

class InventoryItem {
public:
    std::string id;
    int quantity{};
    BEGIN_FIELD_REGISTRATION(InventoryItem)
        REGISTER_FIELD(id)
        REGISTER_FIELD(quantity)
        END_FIELD_REGISTRATION()
};

class Stats {
public:
    int strength{};
    int agility{};
    int intelligence{};
    BEGIN_FIELD_REGISTRATION(Stats)
        REGISTER_FIELD(strength)
        REGISTER_FIELD(agility)
        REGISTER_FIELD(intelligence)
        END_FIELD_REGISTRATION()
};

class CharacterBase {
public:
    std::string name;
    int health{};
    Vec3 position;
    BEGIN_FIELD_REGISTRATION(CharacterBase)
        REGISTER_FIELD(name)
        REGISTER_FIELD(health)
        REGISTER_FIELD(position)
        END_FIELD_REGISTRATION()
};

class Character : public CharacterBase {
public:
    Stats stats;
    Weapon weapon;
    BEGIN_FIELD_REGISTRATION(Character, CharacterBase)
        REGISTER_FIELD(stats)
        REGISTER_FIELD(weapon)
        END_FIELD_REGISTRATION()
};

class PlayerCharacter : public Character {
public:
    std::string playerID;
    InventoryItem item;
    BEGIN_FIELD_REGISTRATION(PlayerCharacter, Character)
        REGISTER_FIELD(playerID)
        REGISTER_FIELD(item)
        END_FIELD_REGISTRATION()
};

class SuperEntity : public Character, public InventoryItem, public Vec3 {
public:
    std::string tag;
    int score{};
    BEGIN_FIELD_REGISTRATION(SuperEntity, Character, InventoryItem, Vec3)
        REGISTER_FIELD(tag)
        REGISTER_FIELD(score)
        END_FIELD_REGISTRATION()
};

class Bag {
public:
    std::vector<std::string> names;
    std::list<int> values;
    std::map<std::string, int> dict;
    std::unordered_map<int, std::string> idMap;

    BEGIN_FIELD_REGISTRATION(Bag)
        REGISTER_FIELD(names)
        REGISTER_FIELD(values)
        REGISTER_FIELD(dict)
        REGISTER_FIELD(idMap)
        END_FIELD_REGISTRATION()
};

class Team {
public:
    std::vector<PlayerCharacter> members;
    std::map<std::string, SuperEntity> bosses;

    BEGIN_FIELD_REGISTRATION(Team)
        REGISTER_FIELD(members)
        REGISTER_FIELD(bosses)
        END_FIELD_REGISTRATION()
};

class Player {
public:
    std::string name;
    int level{};
    float experience{};

    BEGIN_FIELD_REGISTRATION(Player)
        REGISTER_FIELD(name)
        REGISTER_FIELD(level)
        REGISTER_FIELD(experience)
        END_FIELD_REGISTRATION()
};

class PointerHolder {
public:
    std::shared_ptr<Weapon> weapon1;
    std::shared_ptr<Weapon> weapon2;
    std::weak_ptr<Weapon> weaponWeak;
    std::unique_ptr<Weapon> uniqWeapon;
    Weapon* rawWeapon = nullptr;

    BEGIN_FIELD_REGISTRATION(PointerHolder)
        REGISTER_FIELD(weapon1)
        REGISTER_FIELD(weapon2)
        REGISTER_FIELD(weaponWeak)
        REGISTER_FIELD(rawWeapon)
        REGISTER_FIELD(uniqWeapon)
        END_FIELD_REGISTRATION()
};

class Node {
public:
    std::string name;
    std::shared_ptr<Node> next;

    BEGIN_FIELD_REGISTRATION(Node)
        REGISTER_FIELD(name)
        REGISTER_FIELD(next)
        END_FIELD_REGISTRATION()
};

class Outer;

class Inner {
public:
    int value = 2;
    Outer* owner = nullptr;

    BEGIN_FIELD_REGISTRATION(Inner)
        REGISTER_FIELD(value)
        REGISTER_FIELD(owner)
        END_FIELD_REGISTRATION()
};

class Outer {
public:
    std::string name;
    std::unique_ptr<Inner> inner;

    BEGIN_FIELD_REGISTRATION(Outer)
        REGISTER_FIELD(name)
        REGISTER_FIELD(inner)
        END_FIELD_REGISTRATION()
};


void RunInsaneHierarchyPointerTest() {
    using Container = std::vector<std::list<std::unordered_map<int, std::shared_ptr<SuperEntity>>>>;

    SuperEntity* base = new SuperEntity();
    base->name = "Superman";
    base->health = 777;
    base->weapon.name = "LaserVision";
    base->stats.agility = 300;
    base->tag = "alpha";

    std::shared_ptr<SuperEntity> ptr(base);

    Container crazyContainer;
    std::unordered_map<int, std::shared_ptr<SuperEntity>> map;
    map[0] = ptr;
    map[1] = ptr;

    std::list<std::unordered_map<int, std::shared_ptr<SuperEntity>>> lst;
    lst.push_back(map);
    lst.push_back(map);

    crazyContainer.push_back(lst);

    BinarySerializer ser;
    ser.LoadFromObjects(crazyContainer);

    Container loaded;
    ser.DeserializeToObjects(loaded);

    std::cout << "\nDeserialized Insane Hierarchy Container:\n";
    for (auto& lst : loaded)
        for (auto& map : lst)
            for (auto& [k, v] : map)
                std::cout << k << ": " << v->name << " [" << v.get() << "]\n";

    assert(loaded[0].front()[0] == loaded[0].front()[1]);
}

void RunOwnerReferenceTest() {
    std::unique_ptr<Outer> original = std::make_unique<Outer>();
    original->name = "Container";
    original->inner = std::make_unique<Inner>();
    original->inner->owner = original.get();
    original->inner->value = 999;

    BinarySerializer ser;
    ser.LoadFromObjects(original);

    std::unique_ptr<Outer> loaded;
    ser.DeserializeToObjects(loaded);

    std::cout << "\nDeserialized Outer with Inner:\n";
    std::cout << "Outer Name: " << loaded->name << "\n";
    std::cout << "Inner Value: " << loaded->inner->value << "\n";

    assert(loaded->name == "Container");
    assert(loaded->inner->value == 999);
    assert(loaded->inner->owner == loaded.get());
}


void RunSharedControlBlockTest() {
    using ContainerType = std::map<std::string, std::vector<std::shared_ptr<Weapon>>>;

    auto sharedWeapon1 = std::make_shared<Weapon>();
    sharedWeapon1->name = "Excalibur";
    sharedWeapon1->damage = 100;

    auto sharedWeapon2 = std::make_shared<Weapon>();
    sharedWeapon2->name = "Mjolnir";
    sharedWeapon2->damage = 200;

    ContainerType container;
    container["heroes"] = { sharedWeapon1, sharedWeapon1 };
    container["gods"] = { sharedWeapon2, sharedWeapon2, sharedWeapon1 };

    BinarySerializer ser;
    ser.LoadFromObjects(container);

    ContainerType loaded;
    ser.DeserializeToObjects(loaded);

    std::cout << "\nDeserialized Map<string, vector<shared_ptr<Weapon>>>:\n";
    for (auto& [k, vec] : loaded) {
        std::cout << k << ":\n";
        for (auto& w : vec) {
            std::cout << "  Weapon: " << w->name << ", Damage: " << w->damage
                << ", ptr: " << w.get() << "\n";
        }
    }

    auto& heroVec = loaded["heroes"];
    auto& godVec = loaded["gods"];

    assert(heroVec[0] == heroVec[1]);
    assert(godVec[2] == heroVec[0]);
    assert(godVec[0] == godVec[1]);

    std::cout << "Use count of heroVec[0]: " << heroVec[0].use_count() << "\n";
    std::cout << "Use count of godVec[0]: " << godVec[0].use_count() << "\n";

    assert(heroVec[0].use_count() == 3);
    assert(godVec[0].use_count() == 2);
}

void RunVectorOfUniquePtrTest() {
    std::vector<std::unique_ptr<Weapon>> weapons;
    auto w1 = std::make_unique<Weapon>();
    w1->name = "Blaster"; w1->damage = 10;
    weapons.push_back(std::move(w1));

    auto w2 = std::make_unique<Weapon>();
    w2->name = "Phaser"; w2->damage = 20;
    weapons.push_back(std::move(w2));

    BinarySerializer ser;
    ser.LoadFromObjects(weapons);

    std::vector<std::unique_ptr<Weapon>> loaded;
    ser.DeserializeToObjects(loaded);

    std::cout << "\nDeserialized vector<unique_ptr<Weapon>>:\n";
    for (auto& w : loaded) {
        std::cout << "Weapon: " << w->name << ", Damage: " << w->damage << "\n";
    }

    assert(loaded.size() == 2);
    assert(loaded[1]->name == "Phaser");
}

void RunCyclicSharedPtrTest() {
    auto node1 = std::make_shared<Node>();
    auto node2 = std::make_shared<Node>();
    node1->name = "First";
    node2->name = "Second";

    node1->next = node2;
    node2->next = node1;

    BinarySerializer ser;
    ser.LoadFromObjects(node1);

    std::shared_ptr<Node> loaded;
    ser.DeserializeToObjects<std::shared_ptr<Node>>(loaded);

    std::cout << "\nDeserialized Cyclic Node:\n";
    std::cout << "Node1: " << loaded->name << "\n";
    std::cout << "Node2: " << loaded->next->name << "\n";
    std::cout << "Back to Node1: " << loaded->next->next->name << "\n";

    assert(loaded->next->next == loaded);
}

void RunSharedRawPointerTest() {
    auto shared = std::make_shared<Weapon>();
    shared->name = "Laser";
    shared->damage = 123;

    PointerHolder obj;
    obj.weapon1 = shared;
    obj.weapon2 = shared;
    obj.weaponWeak = shared;

    obj.uniqWeapon = std::make_unique<Weapon>();
    obj.uniqWeapon->damage = 332;
    obj.uniqWeapon->name = "daw";

    obj.rawWeapon = obj.uniqWeapon.get();

    BinarySerializer ser;
    ser.LoadFromObjects(obj);

    PointerHolder loaded;
    ser.DeserializeToObjects(loaded);

    std::cout << "\nDeserialized PointerHolder:\n";
    std::cout << "weapon1: " << loaded.weapon1->name << "\n";
    std::cout << "weapon2: " << loaded.weapon2->name << "\n";
    std::cout << "rawWeapon: " << loaded.rawWeapon->name << "\n";
    std::cout << "uniqWeapon: " << loaded.uniqWeapon->name << "\n";

    assert(loaded.weapon1 == loaded.weapon2);
    assert(loaded.weaponWeak.lock() == loaded.weapon1);
    assert(loaded.rawWeapon == loaded.uniqWeapon.get());
    assert(loaded.uniqWeapon->name == obj.uniqWeapon->name);
}

void RunNestedVectorMapTest() {
    using ContainerType = std::vector<std::map<int, std::vector<Player>>>;
    ContainerType data;

    Player p1{ "Alice", 10, 120.5f };
    Player p2{ "Bob", 15, 245.0f };
    Player p3{ "Charlie", 20, 512.25f };

    std::map<int, std::vector<Player>> map1;
    map1[1] = { p1, p2 };
    map1[2] = { p3 };

    std::map<int, std::vector<Player>> map2;
    map2[3] = { p2 };
    map2[4] = { p1, p3 };

    data.push_back(map1);
    data.push_back(map2);

    BinarySerializer ser;
    ser.LoadFromObjects(data);

    ContainerType loaded;
    ser.DeserializeToObjects(loaded);

    std::cout << "\nDeserialized std::vector<std::map<int, std::vector<Player>>>:\n";
    for (size_t i = 0; i < loaded.size(); ++i) {
        std::cout << "Map #" << i << "\n";
        for (auto& [key, vec] : loaded[i]) {
            std::cout << "  Key: " << key << "\n";
            for (auto& player : vec) {
                std::cout << "    Player: " << player.name << ", Level: " << player.level
                    << ", XP: " << player.experience << "\n";
            }
        }
    }

    assert(loaded.size() == 2);
    assert(loaded[0].count(1) > 0);
    assert(loaded[1][4][1].name == "Charlie");
}
void RunContainerWithDerivedTest() {
    Team team;

    PlayerCharacter p1;
    p1.name = "Alice";
    p1.health = 120;
    p1.playerID = "alice_1";
    p1.weapon.name = "Dagger";

    PlayerCharacter p2;
    p2.name = "Bob";
    p2.health = 100;
    p2.playerID = "bob_2";
    p2.weapon.name = "Hammer";

    team.members.push_back(p1);
    team.members.push_back(p2);

    SuperEntity boss;
    boss.name = "Dragon";
    boss.health = 999;
    boss.tag = "FinalBoss";
    team.bosses["dragon"] = boss;

    BinarySerializer ser;
    ser.LoadFromObjects(team);

    Team loaded;
    ser.DeserializeToObjects(loaded);

    std::cout << "\nDeserialized Team:\n";
    for (auto& m : loaded.members)
        std::cout << "Member: " << m.name << ", ID: " << m.playerID << ", Weapon: " << m.weapon.name << "\n";

    for (auto& [k, b] : loaded.bosses)
        std::cout << "Boss: " << k << " => " << b.name << " (Tag: " << b.tag << ")\n";

    assert(loaded.members.size() == 2);
    assert(loaded.members[0].name == "Alice");
    assert(loaded.members[1].weapon.name == "Hammer");
    assert(loaded.bosses.count("dragon") > 0);
    assert(loaded.bosses["dragon"].tag == "FinalBoss");
}

void RunContainerTest() {
    Bag bag;
    bag.names = { "alpha", "beta", "gamma" };
    bag.values = { 1, 2, 3, 4 };
    bag.dict = { {"one", 1}, {"two", 2} };
    bag.idMap = { {100, "sword"}, {200, "shield"} };

    BinarySerializer ser;
    ser.LoadFromObjects(bag);
   
    Bag loaded;
    ser.DeserializeToObjects(loaded);

    std::cout << "\nDeserialized Bag:\n";
    for (auto& name : loaded.names)
        std::cout << "Name: " << name << "\n";

    for (auto& val : loaded.values)
        std::cout << "Value: " << val << "\n";

    for (auto& [k, v] : loaded.dict)
        std::cout << "Dict: " << k << " => " << v << "\n";

    for (auto& [id, item] : loaded.idMap)
        std::cout << "IdMap: " << id << " => " << item << "\n";

    assert(loaded.names.size() == bag.names.size());
    assert(loaded.dict["one"] == 1);
    assert(loaded.idMap[200] == "shield");
}

void RunBigAutoTest() {
    PlayerCharacter pc;
    pc.name = "KnightRider";
    pc.health = 150;
    pc.position = { 10.5f, 20.0f, -5.5f };
    pc.stats = { 15, 12, 9 };
    pc.weapon = { "Sword of Doom", 45, 3.4f };
    pc.playerID = "player_123";
    pc.item = { "health_potion", 5 };

    BinarySerializer ser;
    ser.LoadFromObjects(pc);

    PlayerCharacter loaded;
    ser.DeserializeToObjects(loaded);

    std::cout << "Deserialized PlayerCharacter:\n";
    std::cout
        << "Name: " << loaded.name << "\n"
        << "Health: " << loaded.health << "\n"
        << "Position: (" << loaded.position.x << ", " << loaded.position.y << ", " << loaded.position.z << ")\n"
        << "Stats: [STR=" << loaded.stats.strength
        << ", AGI=" << loaded.stats.agility
        << ", INT=" << loaded.stats.intelligence << "]\n"
        << "Weapon: " << loaded.weapon.name << " (DMG=" << loaded.weapon.damage << ", Weight=" << loaded.weapon.weight << ")\n"
        << "Player ID: " << loaded.playerID << "\n"
        << "Inventory Item: " << loaded.item.id << " x" << loaded.item.quantity << "\n";

    assert(loaded.name == pc.name);
    assert(loaded.position.z == pc.position.z);
    assert(loaded.weapon.name == pc.weapon.name);
    assert(loaded.item.quantity == pc.item.quantity);
}

void RunSuperEntityTest() {
    SuperEntity se;
    se.name = "MegaZord";
    se.health = 999;
    se.position = { 1.1f, 2.2f, 3.3f };
    se.stats = { 100, 200, 300 };
    se.weapon.name = "FusionCannon";
    se.weapon.damage = 9999;
    se.weapon.weight = 99.9f;
    se.id = "artifact_X";
    se.quantity = 42;
    se.tag = "boss_unit";
    se.score = 1'000'000;

    BinarySerializer ser;
    ser.LoadFromObjects(se);

    SuperEntity loaded;
    ser.DeserializeToObjects(loaded);

    std::cout << "\nDeserialized SuperEntity:\n";
    std::cout << "Name: " << loaded.name << "\n";
    std::cout << "Health: " << loaded.health << "\n";
    std::cout << "Position: (" << loaded.position.x << ", " << loaded.position.y << ", " << loaded.position.z << ")\n";
    std::cout << "Stats: STR=" << loaded.stats.strength << ", AGI=" << loaded.stats.agility << ", INT=" << loaded.stats.intelligence << "\n";
    std::cout << "Weapon: " << loaded.weapon.name << " (DMG=" << loaded.weapon.damage << ", Weight=" << loaded.weapon.weight << ")\n";
    std::cout << "Item ID: " << loaded.id << ", Qty: " << loaded.quantity << "\n";
    std::cout << "Tag: " << loaded.tag << ", Score: " << loaded.score << "\n";

    assert(loaded.name == se.name);
    assert(loaded.stats.agility == se.stats.agility);
    assert(loaded.id == se.id);
    assert(loaded.weapon.damage == se.weapon.damage);
    assert(loaded.score == se.score);
}

void MultipleSerializedObjectsTest() {
    BinarySerializer ser;

    Player player;
    Weapon weapon;
    Stats stats;

    player.name = "PlayerX";
    weapon.name = "Gun";
    stats.strength = 100;

    ser.LoadFromObjects(player, weapon, stats);

    Player playerL;
    Weapon weaponL;
    Stats statsL;

    playerL.name;
    weaponL.name;
    statsL.strength;

    ser.DeserializeToObjects(playerL, weaponL, statsL);

    assert(playerL.name==player.name);
    assert(weaponL.name== weapon.name);
    assert(statsL.strength == stats.strength);
}


void RunAllSerializationsTests() {
    RunBigAutoTest();
    RunSuperEntityTest();
    RunContainerTest();
    RunContainerWithDerivedTest();
    RunNestedVectorMapTest();
    RunSharedRawPointerTest();
    RunVectorOfUniquePtrTest();
    RunSharedControlBlockTest();
    RunCyclicSharedPtrTest();
    RunOwnerReferenceTest();
    RunInsaneHierarchyPointerTest();
    MultipleSerializedObjectsTest();

    std::cout << "\Serializer tests passed!\n";
}
