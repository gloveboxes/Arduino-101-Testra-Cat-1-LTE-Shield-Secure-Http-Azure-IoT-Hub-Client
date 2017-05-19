#r "Microsoft.WindowsAzure.Storage"
#r "System.Configuration"
#r "System.Drawing"
#r "Newtonsoft.Json"
using System.Net;
using System.Drawing;
using Microsoft.Azure; // Namespace for CloudConfigurationManager
using Microsoft.WindowsAzure.Storage;
using Microsoft.WindowsAzure.Storage.Blob;
using Microsoft.WindowsAzure.Storage.Table;
using Microsoft.WindowsAzure.Storage.Queue; // Namespace for Queue storage types
using System;
using System.Text;
using Newtonsoft.Json;
using Newtonsoft.Json.Serialization;
using Newtonsoft.Json.Linq;

static string CloudAccountKey = "DefaultEndpointsProtocol=https;AccountName=sheeptalk;AccountKey=4DokGBP7LPE0Pt1bRzWAkqijHJSZF4ihvlwx1g8ID6+S1E2BKvj3RJk+zfBIRkUyqiovceZ2o9o0SCEUW8+gmg==;EndpointSuffix=core.windows.net";
static Random rnd = new Random();

public class AlertQueueEntity
{
    public string Id { get; set; }
    public string Image { get; set; }
}

public class ScoringEntity
{
    public ScoringEntity(string url, string score, string sheepId)
    {
        this.url = url;
        this.Score = score;
        this.sheepId = sheepId;
    }
    [JsonProperty("url", Required = Required.Always)]
    public string url { get; set; }
    [JsonProperty("Score", Required = Required.Always)]
    public string Score { get; set; }
    [JsonProperty("SheepId", Required = Required.Always)]
    public string sheepId { get; set; }
}

public static CloudQueueMessage Serialize(Object o)
{
    return new CloudQueueMessage(JsonConvert.SerializeObject(o));
}

public static void Run(string myQueueItem, TraceWriter log)
{
  //  log.Info($"C# ServiceBus queue trigger function processed message: {myQueueItem}");

    var alert = JsonConvert.DeserializeObject<AlertQueueEntity>(myQueueItem, new JsonSerializerSettings() {ContractResolver = new CamelCasePropertyNamesContractResolver()});

    var imageName = WriteToBlob("-" + alert.Id + "-" + rnd.Next(10000, 500000).ToString(), "sheep", alert.Image);

    var imageUrl = $"https://sheeptalk.blob.core.windows.net/uploaded-images/{imageName}";

    var json = ScoreImage(imageUrl);
    var result = ParseCustomVisionResult(json);


    var ScoringEntity = new ScoringEntity(imageUrl, result, alert.Id);

    WriteToQueue(ScoringEntity, log);
}

static string WriteToBlob(string id, string userId, string base64String)
{
    string status = string.Empty;

    CloudStorageAccount storageAccount = CloudStorageAccount.Parse(CloudAccountKey);
    CloudBlobClient blobClient = storageAccount.CreateCloudBlobClient();
    CloudBlobContainer blobContainer = blobClient.GetContainerReference("uploaded-images");
    blobContainer.CreateIfNotExists();

    string blobName = $"{userId}{id}.jpg";
    CloudBlockBlob blob = blobContainer.GetBlockBlobReference(blobName);
    blob.Properties.ContentType = "image/jpeg";

    try
    {
        byte[] imageBytes = Convert.FromBase64String(base64String);
        // Convert byte[] to Image
        using (var ms = new MemoryStream(imageBytes, 0, imageBytes.Length))
        {
            blob.UploadFromStream(ms);
        }
    }
    catch (StorageException e)
    {
        return $"Error {e.Message}";
    }
    return blobName;
}

static void WriteToQueue(ScoringEntity entity, TraceWriter log)
{
    // Retrieve storage account from connection string.
    CloudStorageAccount storageAccount = CloudStorageAccount.Parse(CloudAccountKey);

    // Create the queue client.
    CloudQueueClient queueClient = storageAccount.CreateCloudQueueClient();

    // Retrieve a reference to a container.
    CloudQueue queue = queueClient.GetQueueReference("image-scoring");

    // Create the queue if it doesn't already exist
    queue.CreateIfNotExists();

    // Create a message and add it to the queue.
    queue.AddMessage(Serialize(entity));
    queue.FetchAttributes();
}

public static string ParseCustomVisionResult(string json)
{
    var predictions =  JObject.Parse(json)["Predictions"].ToString();

    JArray predictionsArray = JArray.Parse(predictions);
    StringBuilder sb = new StringBuilder();

    foreach(var item in predictionsArray)
    {
        sb.Append(item["Tag"] + ": " + item["Probability"]);
        sb.Append(", ");
    }
    sb.Remove(sb.Length - 2, 2); // get rid of last comma

    return sb.ToString();  
}

public static String ScoreImage(string imageUrl)
{
    var cvurl = new Uri("https://southcentralus.api.cognitive.microsoft.com/customvision/v1.0/Prediction/298116ad-7bde-4541-b88e-bcd0734f1924/url?iterationId=cbfb3a77-91b1-4736-9c1f-53a9ed067cb4");

    var httpWebRequest = (HttpWebRequest)WebRequest.Create(cvurl);
    httpWebRequest.Headers.Add("Prediction-Key: 94aac86f3e82418fb9c251d57132a622");
    httpWebRequest.ContentType = "application/json";
    httpWebRequest.Method = "POST";

    using (var streamWriter = new StreamWriter(httpWebRequest.GetRequestStream()))
    {
        string json = "{ \"Url\": \"" + imageUrl + "\"}";

        streamWriter.Write(json);
        streamWriter.Flush();
        streamWriter.Close();
    }

    var httpResponse = (HttpWebResponse)httpWebRequest.GetResponse();
    using (var streamReader = new StreamReader(httpResponse.GetResponseStream()))
    {
        return streamReader.ReadToEnd();
    }
}