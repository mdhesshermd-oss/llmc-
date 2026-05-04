import xml.etree.ElementTree as ET
import json
import math

def load_json(filename):
    try:
        with open(filename, 'r') as f:
            return json.load(f)
    except FileNotFoundError:
        return {}

item_tiers = load_json('item_tiers.json')
entities_to_disable = load_json('entities_to_disable.json')
large_items = set(load_json('large_items.json'))
zombies = set(entities_to_disable.get('zombies', []))
static_objects = set(entities_to_disable.get('static_objects', []))

tree = ET.parse('types.xml')
root = tree.getroot()

# Tier-based defaults
TIER_DEFAULTS = {
    5: {'nominal': 2, 'min': 1, 'value': 'Tier5'},
    4: {'nominal': 10, 'min': 5, 'value': 'Tier4'},
    3: {'nominal': 35, 'min': 20, 'value': 'Tier3'},
    2: {'nominal': 75, 'min': 40, 'value': 'Tier2'},
    1: {'nominal': 120, 'min': 80, 'value': 'Tier1'}
}

# Items to definitely disable based on name patterns if not already caught
def should_disable(name):
    if name in zombies or name in static_objects:
        if name != 'StaticObj_Wreck_HMMWV_DE':
            return True
    if name.startswith('ZmbM_') or name.startswith('ZmbF_'):
        if name != 'StaticObj_Wreck_HMMWV_DE':
            return True
    return False

for t in root.findall('type'):
    name = t.get('name')

    if should_disable(name):
        for tag in ['nominal', 'min']:
            elem = t.find(tag)
            if elem is not None:
                elem.text = '0'
            else:
                new_elem = ET.SubElement(t, tag)
                new_elem.text = '0'
        # Also set restock and lifetime to 0 for these?
        # Actually nominal 0 is enough to stop spawning.
        continue

    # Preservation for HMMWV
    if name == 'StaticObj_Wreck_HMMWV_DE':
        continue

    tier = item_tiers.get(name)
    if tier is None:
        # If not in our scoring list, maybe it's something we should leave alone or default to Tier 1
        # but let's check if it's "0" or other weird ones
        if name == "0":
             # Likely a bug in original file, disable it
             for tag in ['nominal', 'min']:
                elem = t.find(tag)
                if elem is not None: elem.text = '0'
             continue
        tier = 1

    defaults = TIER_DEFAULTS[tier]

    nominal = defaults['nominal']
    min_val = defaults['min']

    # Apply 30% reduction for large items (area >= 21)
    if name in large_items:
        nominal = int(math.ceil(nominal * 0.7))
        min_val = int(math.ceil(min_val * 0.7))

    # Final safety check
    if min_val > nominal:
        min_val = nominal
    if nominal < 1 and name != 'StaticObj_Wreck_HMMWV_DE': # Ensure at least 1 if not disabled
        nominal = 1
        if min_val < 1: min_val = 1

    # Update XML elements
    for tag, val in [('nominal', nominal), ('min', min_val), ('lifetime', 14400), ('restock', 1800)]:
        elem = t.find(tag)
        if elem is not None:
            elem.text = str(val)
        else:
            new_elem = ET.SubElement(t, tag)
            new_elem.text = str(val)

    # Update flags
    flags = t.find('flags')
    if flags is not None:
        flags.set('count_in_hoarder', '0')
        flags.set('count_in_player', '0')
        # We keep count_in_map and count_in_cargo as they are, or ensure map is 1
        flags.set('count_in_map', '1')
    else:
        new_flags = ET.SubElement(t, 'flags')
        new_flags.set('count_in_cargo', '0')
        new_flags.set('count_in_hoarder', '0')
        new_flags.set('count_in_map', '1')
        new_flags.set('count_in_player', '0')
        new_flags.set('crafted', '0')
        new_flags.set('deloot', '0')

    # Update values
    for val_tag in t.findall('value'):
        t.remove(val_tag)
    new_val = ET.SubElement(t, 'value')
    new_val.set('name', defaults['value'])

    # Tier 5 gets Underground usage
    if tier == 5:
        has_underground = False
        for usage in t.findall('usage'):
            if usage.get('name') == 'Underground':
                has_underground = True
                break
        if not has_underground:
            new_usage = ET.SubElement(t, 'usage')
            new_usage.set('name', 'Underground')

# Write with proper formatting (optional but good)
# ET.indent(tree, space="  ", level=0) # Requires Python 3.9+
try:
    ET.indent(tree, space="  ", level=0)
except AttributeError:
    pass # Older python version

tree.write('types.xml', encoding='UTF-8', xml_declaration=True)
print("Successfully updated types.xml")
