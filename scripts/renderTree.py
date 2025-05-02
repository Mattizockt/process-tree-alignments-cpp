import json
import re
from pathlib import Path
import graphviz

# Define a simple class to hold node data during parsing
class TreeNode:
    def __init__(self, node_id, operation, activity, indent):
        self.id = node_id
        self.operation = operation
        self.activity = activity
        self.indent = indent
        self.parent_id = None
        self.children_ids = []

# Mapping for process tree operations (using your updated map)
OPERATION_MAP = {
    0: 'SEQUENCE', # Sequence
    1: 'AND', # AND
    2: 'XOR', # XOR
    3: 'LOOP', # Loop
    4: 'XOR LOOP', # Loop
    5: 'ACTIVITY', # Leaf activity
    6: 'SILENT ACTIVITY' # Silent Activity
}

def parse_tree_file(filepath):
    """
    Parses the custom tree file format and builds a tree structure.

    Args:
        filepath (str or Path): Path to the tree definition file.

    Returns:
        dict: A dictionary mapping node_id to TreeNode object.
        list: A list of node_ids in the order they appeared (useful for root).
    """
    nodes = {}
    node_order = []
    # Keep track of the last node seen at each indentation level
    last_node_at_indent = {}

    # Adjusted pattern to match your format
    indent_pattern = re.compile(r'^(\s*)Node ID: ([a-f0-9-]+), Operation: (\d+), Activity \(if exists\): (.*)')

    try:
        with open(filepath, 'r') as f:
            for line in f:
                match = indent_pattern.match(line)
                if not match:
                    # print(f"Skipping line (does not match pattern): {line.strip()}") # Optional: uncomment for debugging
                    continue

                indent_str, node_id, operation_str, activity_str = match.groups()
                indent = len(indent_str) // 2 # Assuming 2 spaces per indent level
                operation = int(operation_str)
                activity = activity_str.strip() if activity_str.strip() != 'None' else None

                node = TreeNode(node_id, operation, activity, indent)
                nodes[node_id] = node
                node_order.append(node_id)

                # Determine parent based on indentation rule: 1 level up (as per your fixed code)
                parent_indent = indent - 1
                if parent_indent >= 0:
                    # The parent is the last node processed at the parent_indent level
                    parent_id = last_node_at_indent.get(parent_indent)
                    if parent_id and parent_id in nodes:
                        node.parent_id = parent_id
                        nodes[parent_id].children_ids.append(node_id)
                    # else: This node might be the root of a subgraph if parent_indent > 0 but no parent found

                # Update the last node seen at the current indentation level
                last_node_at_indent[indent] = node_id

                # Clean up entries in last_node_at_indent for levels deeper than the current node's parent
                # This handles the case where the structure goes back up the tree
                levels_to_clear = [lvl for lvl in last_node_at_indent if lvl > indent]
                for lvl in levels_to_clear:
                    del last_node_at_indent[lvl]


    except FileNotFoundError:
        print(f"Error: Tree file not found at {filepath}")
        return None, None
    except Exception as e:
        print(f"Error parsing tree file: {e}")
        return None, None

    return nodes, node_order

def parse_percentage_json(filepath):
    """
    Parses the JSON file containing node percentages.

    Args:
        filepath (str or Path): Path to the JSON file.

    Returns:
        dict: A dictionary mapping node_id to percentage (float).
    """
    try:
        with open(filepath, 'r') as f:
            return json.load(f)
    except FileNotFoundError:
        print(f"Error: JSON file not found at {filepath}")
        return {}
    except json.JSONDecodeError:
        print(f"Error: Could not decode JSON from {filepath}")
        return {}
    except Exception as e:
        print(f"Error parsing JSON file: {e}")
        return {}

def map_percentage_to_attributes(percentage):
    percentage = max(0.0, min(100.0, percentage))

    hue = 0.6
    saturation = 0.6 # Full saturation for vibrancy

    max_lightness = 0.99
    min_lightness = 0.1
    lightness = max_lightness - (percentage / 100.0) * (max_lightness - min_lightness)

    fillcolor_hsl = f"{hue:.2f},{saturation:.2f},{lightness:.2f}" # HSL string format for GraphViz

    lightness_threshold = 0.65
    textcolor = 'black' if lightness > lightness_threshold else 'white'


    # --- Font Size Scaling ---
    # Map 0% to base size (e.g., 10) and 100% to max size (e.g., 16)
    base_font_size = 10
    max_font_size = 18 # Slightly smaller max font to avoid crowding
    font_size = base_font_size + (percentage / 100.0) * (max_font_size - base_font_size)


    attributes = {
        'fillcolor': fillcolor_hsl,
        'style': 'filled', # 'filled' is needed for fillcolor to work
        'fontsize': str(int(font_size)), # Font size needs to be an integer string
        'fontcolor': textcolor, # Set text color based on lightness
        'penwidth': '1.0' # Border width
    }

    return attributes

def get_node_label(node):
    """Determines the label for a node based on its type (activity or operator)."""
    if node.activity:
        return node.activity
    else:
        # Use the mapping for operator nodes
        # Add operation code if map lookup fails
        return OPERATION_MAP.get(node.operation, f'Op {node.operation}')


def visualize_custom_tree(tree_data, node_order, percentage_data, output_path="custom_process_tree"):
    """
    Visualizes the custom process tree with nodes styled by percentage data.

    Args:
        tree_data (dict): Dictionary mapping node_id to TreeNode object.
        node_order (list): List of node_ids in parsing order.
        percentage_data (dict): Dictionary mapping node_id to percentage.
        output_path (str): Base path for the output file (e.g., "output/my_tree").
                           GraphViz will add the file extension.
    """
    if not tree_data:
        print("No tree data to visualize.")
        return

    dot = graphviz.Digraph(comment='Custom Process Tree Visualization', graph_attr={'rankdir': 'TB', 'dpi': '150'}) # TB=Top to Bottom, increased DPI for better resolution

    # Add nodes first
    for node_id in node_order: # Use node_order to ensure consistent processing
        node = tree_data[node_id]
        label = get_node_label(node)

        attributes = {}
        # Set default attributes
        attributes['label'] = label
        # Use shape based on operation type (Activity vs others)
        attributes['shape'] = 'box' if node.operation == 5 else 'ellipse' # Assuming Op 5 is Activity

        attributes['style'] = 'filled'
        attributes['fillcolor'] = 'lightgrey' # Default color for nodes without percentage
        attributes['fontsize'] = '10' # Default font size
        attributes['color'] = 'black' # Default text/border color
        attributes['penwidth'] = '1.0' # Border width


        # Apply customization based on percentage data if available
        if node_id in percentage_data:
            percentage = percentage_data[node_id]
            custom_attributes = map_percentage_to_attributes(percentage)
            attributes.update(custom_attributes) # Overwrite defaults with custom attributes

        # Add the node to the graph
        dot.node(node.id, **attributes)

    # Add edges
    # Ensure edges are drawn from parent to child correctly
    for node_id in node_order:
        node = tree_data[node_id]
        # Check if the node has a parent assigned during parsing
        if node.parent_id and node.parent_id in tree_data:
             dot.edge(node.parent_id, node.id)
        # Optional: Add edge labels if needed based on the tree structure

    # Render the graph
    try:
        output_dir = Path(output_path).parent
        output_dir.mkdir(parents=True, exist_ok=True)
        # Added format='pdf' explicitly, you can change to 'png' if preferred
        dot.render(output_path, format='pdf', view=False, cleanup=True)
        print(f"Customized process tree visualization saved to {output_path}.pdf")
    except graphviz.backend.execute.ExecutableNotFound:
        print(f"Error: GraphViz executable not found.")
        print("Please ensure GraphViz is installed and in your system's PATH.")
    except Exception as e:
        print(f"Error rendering GraphViz: {e}")


# --- Example Usage ---

# Use your actual paths
tree_file_path = "/home/matthias/rwth/ba/process-tree-alignments-cpp/tree_depiction.txt"
json_file_path = "/home/matthias/rwth/ba/process-tree-alignments-cpp/outlier_nodes_times_percent.json"
output_visualization_path = "/home/matthias/rwth/ba/process-tree-alignments-cpp/output/outlier" # Output file prefix

# json_file_path = "/home/matthias/rwth/ba/process-tree-alignments-cpp/normal_nodes_times_percent.json"
# output_visualization_path = "/home/matthias/rwth/ba/process-tree-alignments-cpp/output/normal" # Output file prefix

# --- Main Execution ---
print(f"Parsing tree file: {tree_file_path}")
tree_nodes_data, node_parsing_order = parse_tree_file(tree_file_path)
if not tree_nodes_data:
    print("Failed to parse tree file. Exiting.")
else:
    print(f"Parsed {len(tree_nodes_data)} nodes.")
    print(f"Parsing JSON file: {json_file_path}")
    percentage_info = parse_percentage_json(json_file_path)
    print(f"Loaded percentages for {len(percentage_info)} nodes.")

    print(f"\nVisualizing tree with customizations. Output path: {output_visualization_path}")
    visualize_custom_tree(tree_nodes_data, node_parsing_order, percentage_info, output_path=output_visualization_path)

    print("\nDone.")
    print(f"Check '{output_visualization_path}.pdf' for the visualization.")