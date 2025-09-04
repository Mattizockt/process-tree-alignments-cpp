import numpy as np
from pm4py.objects.log.obj import Trace
from pm4py.objects.process_tree.obj import ProcessTree, Operator
from pm4py.objects.process_tree.utils.generic import is_leaf


def trace_to_tuple(trace):
    return tuple([event["concept:name"] for event in trace])


def is_tau_leaf(tree):
    return is_leaf(tree) and (
        tree.label is None or (type(tree.label) is tuple and tree.label[0] is None)
    )


def get_label(trace, pos):
    pos = min(pos, len(trace) - 1)
    return trace[pos]["concept:name"]


def get_subsequence_dict(pt_node):
    # Check that pt_node is a process tree with ids
    if not type(pt_node.label) is tuple:
        raise ValueError(
            "Subsequence Dict cannot be computed: pt_node is not a process tree with ids"
        )
    # Loop through all subsequences of length at most ssl
    subseq_dict = {}
    subsequence_list = [-1] * len(get_ids_from_process_tree(pt_node))
    for pt_id in get_ids_from_process_tree(pt_node):
        subsequence_list[pt_id] = subseq_dict.copy()
    return subsequence_list


def add_id_to_process_tree(pt_node, counter=0):
    """
    This function adds an id to each node in the process tree
    returns maximum counter value from subtree
    """
    old_label = pt_node._get_label()
    if type(old_label) is tuple:
        return counter
    pt_node._set_label((old_label, counter))

    # Case of n-ary operators:
    if pt_node.operator is not None:
        for child in pt_node.children:
            counter = add_id_to_process_tree(child, counter + 1)
    return counter


def get_letter_dict_of_process_tree(pt_node, d={}):
    label, pt_node_id = pt_node._get_label()
    if pt_node.operator is None:
        d[pt_node_id] = {label} if label is not None else set()
    else:
        letters = set()
        for child in pt_node.children:
            _, child_node_id = child._get_label()
            d = d | get_letter_dict_of_process_tree(child, d)
            letters = letters.union(d[child_node_id])
        d[pt_node_id] = letters
    return d


def trace_to_string(trace):
    return "".join([get_label(trace, i) for i in range(len(trace))])


def get_trace_segments(trace):
    segments = {}
    for i in range(len(trace) + 1):
        for j in range(i, len(trace) + 1):
            segments[(i, j)] = None
    return segments


def dyn_align(pt_node, letters_dict, trace, subsequence_dict):

    # The algorithm expects a trace as a tuple of strings
    assert type(trace) is tuple, "Error: Trace is not a tuple!"
    node_id = pt_node._get_label()[1]

    # Check if costs for short subsequence are already computed in subsequence_dict
    # if len(trace) <= ssl and pt_node.operator is None:
    if trace in subsequence_dict[node_id]:
        return subsequence_dict[node_id][trace]

    original_trace = trace
    # Remove all letters from trace that are not contained in subtree
    # letters_in_tree = [x for x in trace if x in letters_dict[node_id]]
    # unmatched = len(trace) - len(letters_in_tree)
    # if unmatched > 0:
        # trace = tuple(letters_in_tree)

    if pt_node.operator is None:
        if is_tau_leaf(pt_node):
            costs = _dyn_align_tau_leaf(pt_node, letters_dict, trace)
        else:
            costs = _dyn_align_leaf(pt_node, letters_dict, trace)

    elif pt_node.operator in [
        Operator.SEQUENCE,
        Operator.XOR,
        Operator.PARALLEL,
        Operator.LOOP,
    ]:

        # Then call special purpose functions to handle the current node depending on the operator
        match pt_node.operator:
            case Operator.SEQUENCE:  # Concatenation
                costs = _dyn_align_sequence(
                    pt_node, letters_dict, trace, subsequence_dict
                )
            case Operator.XOR:  # Exclusive choice
                costs = _dyn_align_xor(pt_node, letters_dict, trace, subsequence_dict)
            case Operator.PARALLEL:  # Parallel execution
                costs = _dyn_align_shuffle(
                    pt_node, letters_dict, trace, subsequence_dict
                )
            case Operator.LOOP:  # Loop
                costs = _dyn_align_loop(pt_node, letters_dict, trace, subsequence_dict)

    else:
        raise ValueError(f"Operator {pt_node.operator} not supported!")

    # Done, add log moves for unmatched letters
    # costs = unmatched + costs
    # costs = costs

    subsequence_dict[node_id].update({original_trace: costs})

    return costs


def _dyn_align_tau_leaf(pt_node, letters_dict, trace):
    """
    This function aligns a tau leaf node with the trace
    """
    return len(trace)


def _dyn_align_leaf(pt_node, letters_dict, trace):
    """
    This function aligns a leaf node with the trace
    """
    pt_node_label = pt_node._get_label()[0]
    n = len(trace)
    costs = 1 if n == 0 else (n - (1 if pt_node_label in trace else -1))
    return costs


def generate_partitions(n, parts=2):
    """
    This function generates all possible partitions of a set of n elements into parts
    """
    if parts == 0:
        return []
    if parts == 1:
        return [[n]]
    if n == 0:
        return [[0] * parts]
    partitions = []
    for i in range(0, n + 1):
        partitions += [[i] + p for p in generate_partitions(n - i, parts - 1)]
    return partitions


def get_ids_from_process_tree(pt_node):
    if is_leaf(pt_node):
        ids = [pt_node._get_label()[1]]
    else:
        ids = [pt_node._get_label()[1]] + [
            pt_id
            for child in pt_node.children
            for pt_id in get_ids_from_process_tree(child)
        ]
    # sort the ids
    return sorted(ids)


def get_segments_for_sequence(trace, list_of_letters):
    """
    This function computes a list of segments that are sufficient to find an optimal.
    The list_of_letters contains (pairwise disjoint) sets of letters that are contained in the subtrees.
    In particular the number of sets in list_of_letters is equal to the number of children of the sequence node.
    """
    n = len(trace)
    number_of_children = len(list_of_letters)
    if number_of_children > 2:
        # Not implemented yet : Fallback to naive approach (generate all segments), attention: this can be very slow (exponential runtime)
        # Our idea of L-/R-splits should generalize to this case. At the moment, a shortest-path algorithm is used to find the best partition
        return generate_partitions(n, number_of_children)
    elif number_of_children == 2:
        # left_letters = list_of_letters[0]
        right_letters = list_of_letters[1]
        segments = [(0, n), (n, 0)]
        split_positions = [
            i
            for i in range(1, n)
            if trace[i] in right_letters and trace[i - 1] not in right_letters
        ]
        for i in split_positions:
            segments.append((i, n - i))
    return segments


def _dyn_align_sequence(pt_node, letters_dict, trace, subsequence_dict):
    """
    This function aligns a sequence node with the trace
    """
    children = pt_node.children
    number_of_children = len(children)
    n = len(trace)

    if n == 0:
        return sum(
            [
                dyn_align(child, letters_dict, tuple(), subsequence_dict)
                for child in children
            ]
        )

    if number_of_children == 1:
        return dyn_align(children[0], letters_dict, trace, subsequence_dict)
    # Greedy approach
    # Try to split the trace up into subsequences with respect to the occurring letters
    # pos = 0
    # old_pos = 0
    # costs = 0
    # if (
    #     len(trace) > len(children)
    #     and trace[0] in letters_dict[children[0]._get_label()[1]]
    #     and trace[-1] in letters_dict[children[-1]._get_label()[1]]
    # ):
    #     for child in children:
    #         letters_current_child = letters_dict[child._get_label()[1]]
    #         while pos < n and trace[pos] in letters_current_child:
    #             pos += 1
    #         costs += dyn_align(
    #             child, letters_dict, trace[old_pos:pos], subsequence_dict
    #         )
    #         old_pos = pos

    # if pos < n:
    # costs = np.inf  # Greedy approach failed

    # if costs == 0:
    #     return 0
    # else:
    #     upper_bound = costs  # We found an alignment which can serve as an upper_bound

    # Most common case of a binary sequence operator
    if number_of_children == 2:
        segments = get_segments_for_sequence(
            trace, [letters_dict[child._get_label()[1]] for child in children]
        )
        costs = np.inf
        for partition in segments:
            split = partition[0]
            left_cost = dyn_align(
                children[0], letters_dict, trace[:split], subsequence_dict
            )
            # if left_cost > upper_bound or left_cost > costs:
            #     continue
            right_cost = dyn_align(
                children[1], letters_dict, trace[split:], subsequence_dict
            )
            costs = min(costs, left_cost + right_cost)
        # return min(upper_bound, costs)
        return costs

    # Otherwise: define a graph to determine the best partition of the trace
    # vertices are of the form (i,j) where i ranges from 0 to number_of_children (inclusive) and j ranges from 0 to n (inclusive)
    # the idea is that a path (0,0) -> (1,a) -> (2,b) -> ... -> (number_of_children,n) corresponds to a partition of the trace
    # as [0,a], [a,b], [b,c], ...,
    vertices = [
        (i, j)
        for i in range(number_of_children + 1)
        for j in range(n + 1)
        if (
            (i > 0 and i < number_of_children)
            or (i == 0 and j == 0)
            or (i == number_of_children and j == n)
        )
    ]

    def outgoing_edges(v):
        i, j = v
        if i == number_of_children:
            return []
        for k in range(j, n + 1):
            if i == number_of_children - 1 and k < n:
                continue
            # Optimisation for unique label:
            # We check if the letter at position (k+1) is also in the letters of the i-th child
            # If yes, we skip this edge (because it would not be worse to include the next position as well)
            if k < n - 1 and trace[k] in letters_dict[children[i]._get_label()[1]]:
                continue
            tmp_costs = dyn_align(
                children[i], letters_dict, trace[j:k], subsequence_dict
            )
            # if tmp_costs > upper_bound:
            #     continue
            yield ((i, j), (i + 1, k), tmp_costs)

    start = (0, 0)
    end = (number_of_children, n)

    # Now, compute a path from start to end with minimum costs using Dijkstra's algorithm
    costs = {}
    for v in vertices:
        costs[v] = np.inf
    costs[start] = 0
    visited = set()
    while len(visited) < len(vertices):
        current = min([v for v in vertices if v not in visited], key=lambda x: costs[x])
        visited.add(current)
        # if costs[current] > upper_bound:
        #     continue  # There will be no better path to the end vertex along this vertex (since minimal costs exceed upper_bound)
        for edge in outgoing_edges(current):
            costs[edge[1]] = min(costs[edge[1]], costs[current] + edge[2])

    return costs[end]


def _dyn_align_shuffle(pt_node, letters_dict, trace, subsequence_dict):
    """
    This function aligns a shuffle node with the trace
    """
    children = pt_node.children
    n = len(trace)
    if n == 0:
        return sum(
            [
                dyn_align(child, letters_dict, tuple(), subsequence_dict)
                for child in children
            ]
        )
    # For each node, get the subtraces that are aligned with the children
    subtraces = [
        tuple([x for x in trace if x in letters_dict[child._get_label()[1]]])
        for child in children
    ]
    costs = [
        dyn_align(child, letters_dict, subtrace, subsequence_dict)
        for child, subtrace in zip(children, subtraces)
    ]
    # Return the sum of the costs and the number of unmatched events
    unmatched = len(trace) - sum(len(sub_trace) for sub_trace in subtraces)
    return sum(costs)  + unmatched


def _dyn_align_loop(pt_node, letters_dict, trace, subsequence_dict):
    """
    This function aligns a loop node with the trace
    A loop is of the form r (q r)*
    """

    r = pt_node.children[0]
    q = pt_node.children[1]

    n = len(trace)
    if n == 0:
        return dyn_align(r, letters_dict, tuple(), subsequence_dict)

    r_letters = letters_dict[r._get_label()[1]]
    q_letters = letters_dict[q._get_label()[1]]

    # First try to find a split of the trace using a greedy strategy using the unique label property
    # This gives us an upper bound on the costs that we can use to prune the search space
    # costs = np.inf
    # if trace[0] in r_letters and trace[-1] in r_letters:
    #     # In this case, we can split the trace into a sequence of r and a sequence of (QR)*
    #     # We use this sequence to get an upper bound on the costs
    #     i = 0
    #     # Start with the first r part
    #     # Find the first element that is not in r
    #     # This is the end of the first r part
    #     r_parts = []
    #     q_parts = []
    #     while i < n and trace[i] in r_letters:
    #         i += 1
    #     r_parts.append(trace[:i])
    #     while i < n:
    #         # Find the first element that is in r
    #         # This is the start of the next r part
    #         j = i
    #         while j < n and trace[j] not in r_letters:
    #             j += 1
    #         q_parts.append(trace[i:j])
    #         # Next find the first element that is not in r
    #         # This is the end of the next r part
    #         i = j
    #         while i < n and trace[i] in r_letters:
    #             i += 1
    #         r_parts.append(trace[j:i])
    #     # Now, compute the costs of the r parts
    #     costs = sum([dyn_align(r, letters_dict, part, subsequence_dict) for part in r_parts])
    #     # Now, compute the costs of the q parts
    #     costs += sum([dyn_align(q, letters_dict, part, subsequence_dict) for part in q_parts])

    # # Evaluate costs for greedy approach
    # if costs == 0:
    #     return 0
    # else:
    #     upper_bound = costs
    # # No alignment will be worse than this trivial upper bound

    segments = [(i, j) for i in range(n + 1) for j in range(i, n + 1)]
    # Compute costs for QR segments
    empty_trace = 0  # Since we will never use a QR block for an empty trace in an optimal alignment

    cost_qr_segments = {}

    for i, j in segments:
        if i == j:
            cost_qr_segments[(i, j)] = empty_trace
        else:
            costs = np.inf
            for k in get_segments_for_sequence(trace[i:j], [q_letters, r_letters]):
                q_cost = dyn_align(
                    q, letters_dict, trace[i : (i + k[0])], subsequence_dict
                )
                # if q_cost > upper_bound or q_cost > costs:
                #     continue
                r_cost = dyn_align(
                    r, letters_dict, trace[(i + k[0]) : j], subsequence_dict
                )
                # if r_cost > upper_bound or r_cost > costs:
                #     continue
                costs = min(costs, q_cost + r_cost)
                if costs == 0:
                    break
            # if costs < upper_bound:
            cost_qr_segments[(i, j)] = costs
            # else:
            # cost_qr_segments[(i, j)] = np.inf

    # Compute costs for (QR)* segments
    for _ in range(n):
        change = False
        for i, j in segments:
            if i == j:
                continue
            if cost_qr_segments[(i, j)] == 0:
                continue
            old = cost_qr_segments[(i, j)]
            new = min(
                [
                    cost_qr_segments[(i, k)] + cost_qr_segments[(k, j)]
                    for k in range(i, j + 1)
                ],
                default=empty_trace,
            )
            if new < old:
                change = True
                cost_qr_segments[(i, j)] = new
        if not change:
            break

    # Now, compute the r costs:
    cost_r_segments = {
        i: dyn_align(r, letters_dict, trace[:i], subsequence_dict)
        for i in range(n + 1)
        # if cost_qr_segments[(i, n)] <= upper_bound
    }
    # Now, compute the costs for the loop
    costs = min(
        [cost_r_segments[i] + cost_qr_segments[(i, n)] for i in cost_r_segments]
    )
    return costs


def _dyn_align_xor(pt_node, letters_dict, trace, subsequence_dict):
    """
    This function aligns a xor node with the trace
    """
    children = pt_node.children
    costs = dyn_align(children[0], letters_dict, trace, subsequence_dict)
    for child in children[1:]:
        if costs == 0:
            return 0
        costs = min(costs, dyn_align(child, letters_dict, trace, subsequence_dict))
    return costs


if __name__ == "__main__":
    pass
