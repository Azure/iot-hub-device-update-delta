/**
 * @file ConcatenationRecipe.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */


namespace ArchiveUtility
{
    using System;
    using System.Collections.Generic;
    using System.IO;

    public class ConcatenationRecipe : RecipeBase<ConcatenationRecipe>
    {
        public override string Name => "Concatenation";
        public override RecipeType Type => RecipeType.Concatenation;
        protected override List<string> ParameterNames { get; } = new();
        public ConcatenationRecipe() : base() { }
        public ConcatenationRecipe(params ArchiveItem[] archiveItemParameters) : base(archiveItemParameters) { }
        protected override Dictionary<string, RecipeParameter> InitializeParameters(RecipeParameter[] recipeParameters)
        {
            var parameterDictionary = new Dictionary<string, RecipeParameter>();
            if (recipeParameters.Length != 0)
            {
                for (int i = 0; i < recipeParameters.Length; i++)
                {
                    var paramName = i.ToString();
                    ParameterNames.Add(paramName);
                    parameterDictionary[paramName] = recipeParameters[i];
                }
            }
            return parameterDictionary;
        }
        public override void SetParameters(Dictionary<string, RecipeParameter> parameterDictionary)
        {
            for (int i = 0; i < parameterDictionary.Count; i++)
            {
                var paramName = i.ToString();

                if (!parameterDictionary.ContainsKey(paramName))
                {
                    throw new Exception(@$"Expected to find {paramName} in ConcatenationRecipe, but found these instead: {string.Join(',', parameterDictionary.Keys)}");
                }
                ParameterNames.Add(paramName);
            }

            base.SetParameters(parameterDictionary);
        }

        public override void SetParameter(string name, RecipeParameter value)
        {
            int count = ParameterNames.Count;

            string nextName = count.ToString();

            if (name.Equals(nextName))
            {
                ParameterNames.Add(name);
            }
            base.SetParameter(name, value);
        }

        public override void CopyTo(Stream archiveStream, Stream writeStream)
        {
            var parameters = GetParameters();

            foreach (var parameter in parameters)
            {
                var paramName = parameter.Item1;
                var paramValue = parameter.Item2;

                var item = (paramValue as ArchiveItemRecipeParameter).Item;
                item.CopyTo(archiveStream, writeStream);
            }
        }
    }
}
