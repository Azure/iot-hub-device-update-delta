namespace ArchiveUtility
{
    using System;
    using System.Collections.Generic;
    using System.Linq;

    public class PayloadCatalog
    {
        // Sometimes a given payload name has mulitple, conflicting items
        // associated with it
        private Dictionary<Payload, HashSet<ItemDefinition>> _payload = new();

        public HashSet<ItemDefinition> ArchiveItems { get; } = new();

        private Dictionary<string, HashSet<Payload>> _payloadWildcardMatches = new();
        private Dictionary<string, HashSet<Payload>> _payloadNameMatches = new();

        public Dictionary<Payload, HashSet<ItemDefinition>> Entries { get => _payload; }

        public IEnumerable<ItemDefinition> GetPayloadMatchingWildcard(string name)
        {
            string wildcardName = Payload.GetWildcardPayloadName(name);

            List<ItemDefinition> matches = new List<ItemDefinition>();

            if (_payloadWildcardMatches.TryGetValue(wildcardName, out HashSet<Payload> wildcardMatches))
            {
                foreach (var payload in wildcardMatches)
                {
                    var items = _payload[payload];
                    matches.AddRange(items);
                }
            }

            return matches;
        }

        public bool HasPayloadWithName(string name) => _payloadNameMatches.ContainsKey(name);

        public IEnumerable<ItemDefinition> GetPayloadWithName(string name)
        {
            List<ItemDefinition> matches = new List<ItemDefinition>();

            if (_payloadNameMatches.TryGetValue(name, out HashSet<Payload> payloadMatches))
            {
                foreach (var payload in payloadMatches)
                {
                    var items = _payload[payload];
                    matches.AddRange(items);
                }
            }

            return matches;
        }

        public void AddPayload(Payload payload, ItemDefinition item)
        {
            if (!_payload.ContainsKey(payload))
            {
                _payload.Add(payload, new());
            }

            _payload[payload].Add(item);

            string wildcardName = NamePatterns.ReplaceNumbersWithWildCards(payload.Name);

            if (!_payloadWildcardMatches.ContainsKey(wildcardName))
            {
                _payloadWildcardMatches.Add(wildcardName, new());
            }

            if (!_payloadNameMatches.ContainsKey(payload.Name))
            {
                _payloadNameMatches.Add(payload.Name, new());
            }

            _payloadWildcardMatches[wildcardName].Add(payload);
            _payloadNameMatches[payload.Name].Add(payload);

            if (!ArchiveItems.Contains(payload.ArchiveItem))
            {
                ArchiveItems.Add(payload.ArchiveItem);
            }

            //AddLookupsForItem(item);
        }

        public bool HasPayload(Payload payload)
        {
            return _payload.ContainsKey(payload);
        }

        public IEnumerable<ItemDefinition> GetPayload(Payload payload)
        {
            return _payload[payload];
        }
    }
}
