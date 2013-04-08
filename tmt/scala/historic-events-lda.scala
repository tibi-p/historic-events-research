// Stanford TMT
// http://nlp.stanford.edu/software/tmt/0.4/

// tells Scala where to find the TMT classes
import scalanlp.io._;
import scalanlp.stage._;
import scalanlp.stage.text._;
import scalanlp.text.tokenize._;
import scalanlp.pipes.Pipes.global._;

import java.io.File

import edu.stanford.nlp.tmt.stage._;
import edu.stanford.nlp.tmt.model.lda._;
import edu.stanford.nlp.tmt.model.llda._;

def trainHistoricModel(filename: String, modelType: String) = {
  val source = CSVFile(filename) ~> IDColumn(1)

  val tokenizer = {
    SimpleEnglishTokenizer() ~>            // tokenize on space and punctuation
    CaseFolder() ~>                        // lowercase everything
    WordsAndNumbersOnlyFilter() ~>         // ignore non-words and non-numbers
    MinimumLengthFilter(3) ~>              // take terms with >=3 characters
    StopWordFilter("en")
  }

  val text = {
    source ~>                              // read from the source file
    Column(4) ~>                           // select column containing text
    TokenizeWith(tokenizer) ~>             // tokenize with tokenizer above
    TermCounter() ~>                       // collect counts (needed below)
    TermMinimumDocumentCountFilter(4) ~>   // filter terms in <4 docs
    TermDynamicStopListFilter(30) ~>       // filter out 30 most common terms
    DocumentMinimumLengthFilter(5)         // take only docs with >=5 terms
  }

  // turn the text into a dataset ready to be used with LDA
  val dataset = LDADataset(text)

  // define the model parameters
  val params = LDAModelParams(numTopics = 150, dataset = dataset,
    topicSmoothing = 0.01, termSmoothing = 0.01)

  // Name of the output model folder to generate
  val modelPath = file("data/lda/" + modelType + "-" + dataset.signature + "-" + params.signature)

  // Trains the model: the model (and intermediate models) are written to the
  // output folder.  If a partially trained model with the same dataset and
  // parameters exists in that folder, training will be resumed.
  //TrainCVB0LDA(params, dataset, output=modelPath, maxIterations=200);

  // To use the Gibbs sampler for inference, instead use
  TrainGibbsLDA(params, dataset, output=modelPath, maxIterations=300);

  (source, dataset, modelPath)
}

val historyDirectory = new File("data/zeitgeist/history")
for (historyFile <- historyDirectory.listFiles) {
  val filePath = historyFile.getPath
  val name = historyFile.getName
  val modelType = name.substring(0, name.length - "_history.csv".length)
  val modelParams = trainHistoricModel(filePath, modelType)
  val source = modelParams._1
  val dataset = modelParams._2
  val modelPath = modelParams._3

  println("Loading " + modelPath)
  //val model = LoadCVB0LDA(modelPath)
  // Or, for a Gibbs model, use:
  val model = LoadGibbsLDA(modelPath)

  val output = file(modelPath, source.meta[java.io.File].getName.replaceAll(".csv",""))

  // define fields from the dataset we are going to slice against
  val slice = source ~> Column(2)
  // could be multiple columns with: source ~> Columns(2,7,8)

  println("Loading document distributions");
  val perDocTopicDistributions = LoadLDADocumentTopicDistributions(
    CSVFile(modelPath,"document-topic-distributions.csv"));
  // This could be InferDocumentTopicDistributions(model, dataset)
  // for a new inference dataset.  Here we load the training output.

  println("Writing topic usage to "+output+"-sliced-usage.csv");
  val usage = QueryTopicUsage(model, dataset, perDocTopicDistributions, grouping=slice);
  CSVFile(output+"-sliced-usage.csv").write(usage);

  /*
  println("Estimating per-doc per-word topic distributions");
  val perDocWordTopicDistributions = EstimatePerWordTopicDistributions(
    model, dataset, perDocTopicDistributions);

  println("Writing top terms to "+output+"-sliced-top-terms.csv");
  val topTerms = QueryTopTerms(model, dataset, perDocWordTopicDistributions, numTopTerms=50, grouping=slice);
  CSVFile(output+"-sliced-top-terms.csv").write(topTerms);
  */
}
