import xml.etree.ElementTree as ET
import xml.dom.minidom
import re

def get_item_config(name):
    config = {
        "nominal": 0,
        "lifetime": 14400,
        "restock": 1800,
        "min": 0,
        "quantmin": -1,
        "quantmax": -1,
        "cost": 100,
        "flags": {"count_in_cargo": "0", "count_in_hoarder": "0", "count_in_map": "1", "count_in_player": "0", "crafted": "0", "deloot": "0"},
        "category": "exported",
        "usages": set(),
        "values": set()
    }

    name_lower = name.lower()

    # --- ЖЕСТКАЯ ФИЛЬТРАЦИЯ (ИСКЛЮЧАЕМ МУСОР И АВТО) ---
    exclude_patterns = [
        "zmb", "infected", "animal", "bear", "wolf", "hen", "cow", "sheep", "pig", "deer", "rooster", "goat",
        "alien", "captain", "queen", "scout", "ufo", # Пришельцы (идут через events)
        "ship", "land_", "static_", "wreck_", "proxy", "test", "base", "bttstck", "hndgrd", # Статика и запчасти (если не нужны)
        "amggt63", "offroad", "civsedan", "truck", "humve", "hatchback", "sedan", "volga", "ada", "sarka", "gunner", # Машины
        "koleso", "dver", "kapot", "bagazhnik", "wheel", "door", "hood", "trunk", "battery", "sparkplug", "radiator", "headlight" # Запчасти
    ]

    if any(p in name_lower for p in exclude_patterns):
        # Оставляем nominal 0 для этих объектов, чтобы они не спавнились как лут
        return config

    def has_any(keywords):
        return any(kw.lower() in name_lower for kw in keywords)

    # --- РАСПРЕДЕЛЕНИЕ ОРУЖИЯ ПО УРОВНЯМ ---

    # TIER 5 - EXTRA RARE (Bunkers, Heli, Convoy)
    top_tier = ["m82", "gm6", "lynx", "pkm", "pkp", "m249", "dvl", "chey", "intervention", "408", "338", "50bmg", "rsass", "m110", "mdr", "m300", "srs"]
    if has_any(top_tier) or "kod_" in name_lower:
        config["nominal"] = 1
        config["min"] = 1
        config["lifetime"] = 28800
        config["restock"] = 7200
        config["category"] = "weapons"
        config["usages"].add("Military")
        config["values"].update(["Tier5", "Underground"])
        config["flags"]["count_in_hoarder"] = "1"
        config["flags"]["deloot"] = "1"

    # TIER 4 - HIGH END MILITARY
    elif has_any(["ak101", "ak12", "ak15", "akalpha", "m4a1", "asval", "vss", "svd", "fal", "lar", "platecarrier", "nvg"]):
        config["nominal"] = 3
        config["min"] = 1
        config["usages"].add("Military")
        config["values"].add("Tier4")
        config["category"] = "weapons" if "ak" in name_lower or "m4" in name_lower or "val" in name_lower or "vss" in name_lower or "svd" in name_lower else "clothing"

    # TIER 3 - STANDARD MILITARY
    elif has_any(["ak74", "m16", "aug", "famas", "sks", "pressvest", "ballistichelmet"]):
        config["nominal"] = 5
        config["min"] = 2
        config["usages"].add("Military")
        config["values"].update(["Tier2", "Tier3"])
        config["category"] = "weapons"

    # HUNTING (ТОЛЬКО ОХОТА)
    elif has_any(["mosin", "winchester", "tundra", "blaze", "savanna", "cz527", "hunting", "hunter"]):
        config["nominal"] = 4
        config["min"] = 2
        config["usages"].add("Hunting")
        config["category"] = "weapons"

    # POLICE
    elif has_any(["mp5", "ump", "glock", "cz75", "police", "deagle", "p1", "makarov", "ij70"]):
        config["nominal"] = 8
        config["min"] = 4
        config["usages"].add("Police")
        config["category"] = "weapons"

    # MEDIC
    elif has_any(["medic", "bandage", "saline", "epinephrine", "morphine", "firstaid", "blood", "vitamin", "tetracycline"]):
        config["nominal"] = 10
        config["min"] = 5
        config["category"] = "medic"
        config["usages"].add("Medic")

    # FOOD / CIVILIAN (Base)
    elif has_any(["food", "can", "water", "soda", "backpack", "bag"]):
        config["nominal"] = 15
        config["min"] = 8
        config["category"] = "food"
        config["usages"].update(["Town", "Village"])

    # AMMO / MAGS - MATCHING
    if "mag_" in name_lower or "ammo_" in name_lower or "ammobox" in name_lower:
        config["category"] = "weapons"
        if has_any(["50bmg", "408", "338", "300", "pkm", "m249", "kod_"]):
            config["nominal"] = 2
            config["min"] = 1
            config["usages"] = {"Military"}
            config["values"] = {"Tier5", "Underground"}
        elif has_any(["556", "545", "762x39", "762x54", "308"]):
            config["nominal"] = 8
            config["min"] = 4
            config["usages"] = {"Military"}
            config["values"] = {"Tier3", "Tier4"}
        else:
            config["nominal"] = 12
            config["min"] = 6
            config["usages"].update(["Town", "Village", "Police"])

    # Fallback for clothing and tools
    if config["nominal"] == 0 and not any(p in name_lower for p in exclude_patterns):
        if has_any(["jeans", "tshirt", "hoodie", "jacket", "pants", "boots"]):
            config["nominal"] = 10
            config["min"] = 5
            config["category"] = "clothing"
            config["usages"].update(["Town", "Village"])
        elif has_any(["hammer", "wrench", "screwdriver", "pliers", "axe", "saw"]):
            config["nominal"] = 8
            config["min"] = 4
            config["category"] = "tools"
            config["usages"].add("Industrial")

    if not config["usages"] and config["nominal"] > 0:
        config["usages"].update(["Town", "Village"])

    return config

def generate_types_xml(input_file, output_file):
    root = ET.Element("types")

    with open(input_file, "r") as f:
        items = f.read().splitlines()

    count = 0
    for item_name in sorted(items):
        if not item_name.strip(): continue

        config = get_item_config(item_name)

        # Only add to XML if nominal > 0 or it's a critical NPC/Object (with nominal 0)
        # However, to avoid lag, we only include items with nominal > 0
        if config["nominal"] == 0: continue

        type_el = ET.SubElement(root, "type", name=item_name)
        ET.SubElement(type_el, "nominal").text = str(config["nominal"])
        ET.SubElement(type_el, "lifetime").text = str(config["lifetime"])
        ET.SubElement(type_el, "restock").text = str(config["restock"])
        ET.SubElement(type_el, "min").text = str(config["min"])
        ET.SubElement(type_el, "quantmin").text = str(config["quantmin"])
        ET.SubElement(type_el, "quantmax").text = str(config["quantmax"])
        ET.SubElement(type_el, "cost").text = str(config["cost"])
        ET.SubElement(type_el, "flags", **config["flags"])
        ET.SubElement(type_el, "category", name=config["category"])

        for usage in sorted(list(config["usages"])):
            ET.SubElement(type_el, "usage", name=usage)
        for value in sorted(list(config["values"])):
            ET.SubElement(type_el, "value", name=value)
        count += 1

    xml_str = ET.tostring(root, encoding="utf-8")
    dom = xml.dom.minidom.parseString(xml_str)
    pretty_xml = dom.toprettyxml(indent="    ")

    header = '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>\n'
    content = pretty_xml.split('\n', 1)[1]

    with open(output_file, "w", encoding="utf-8") as f:
        f.write(header + content)
    return count

if __name__ == "__main__":
    count = generate_types_xml("current_items.txt", "types_balanced.xml")
    print(f"Generated types_balanced.xml with {count} items")
